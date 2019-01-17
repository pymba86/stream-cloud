
#include <websocket.hpp>
#include "ws_session.hpp"
#include "error.hpp"

namespace stream_cloud {
    namespace providers {
        namespace ws_server {
            constexpr const char *dispatcher = "dispatcher";
            constexpr const char *error = "error";

            void ws_session::on_write(boost::system::error_code ec, std::size_t bytes_transferred) {
                boost::ignore_unused(bytes_transferred);

                if (ec) {
                    return fail(ec, "write");
                }
                // Clear the buffer
                buffer_.consume(buffer_.size());

                // Inform the queue that a write completed
                if (queue_.on_write()) {
                    // Read another request
                    do_read();
                }
            }

            void ws_session::close() {
                boost::system::error_code ec;
                ws_.close(boost::beast::websocket::close_code::normal, ec);
            }

            void ws_session::on_read(boost::system::error_code ec, std::size_t bytes_transferred) {
                boost::ignore_unused(bytes_transferred);

                if (ec == websocket::error::closed) {
                    return;
                }

                if (ec) {

                    auto *error_m = new api::error(id_);
                    error_m->code = ec;
                    main_pipe_->send(
                            messaging::make_message(
                                    main_pipe_,
                                    error,
                                    api::transport(error_m)
                            )
                    );

                    return;
                }

                auto *ws = new api::web_socket(id_);
                ws->body = boost::beast::buffers_to_string(buffer_.data());
                main_pipe_->send(
                        messaging::make_message(
                                main_pipe_,
                                dispatcher,
                                api::transport(ws)
                        )
                );

                buffer_.consume(buffer_.size());

                // If we aren't at the queue limit, try to pipeline another request
                if (!queue_.is_full()) {
                    do_read();
                }
            }

            void ws_session::do_read() {
                ws_.async_read(
                        buffer_,
                        boost::asio::bind_executor(
                                strand_,
                                std::bind(
                                        &ws_session::on_read,
                                        shared_from_this(),
                                        std::placeholders::_1,
                                        std::placeholders::_2
                                )
                        )
                );
            }

            void ws_session::on_accept(boost::system::error_code ec) {
                if (ec) {
                    return fail(ec, "accept");
                }

                do_read();
            }

            void ws_session::run() {
                ws_.async_accept_ex(
                        [](websocket::response_type &res) {

                        },
                        boost::asio::bind_executor(
                                strand_,
                                std::bind(
                                        &ws_session::on_accept,
                                        shared_from_this(),
                                        std::placeholders::_1
                                )
                        )
                );
            }

            ws_session::ws_session(tcp::socket socket, api::transport_id id, actor::actor_address main_pipe) :
                    ws_(std::move(socket)),
                    strand_(ws_.get_executor()),
                    id_(id),
                    main_pipe_(main_pipe),
                    queue_(*this) {
                setup_stream(ws_);
            }

            void ws_session::write(const intrusive_ptr<api::web_socket> &ptr) {

                queue_(ptr->body);
            }

            api::transport_id ws_session::id() const {
                return id_;
            }

            void ws_session::write(const std::string &value) {
                queue_(value);
            };

            bool ws_session::queue::on_write() {
                BOOST_ASSERT(!items_.empty());
                auto const was_full = is_full();
                items_.erase(items_.begin());
                if (!items_.empty())
                    (*items_.front())();
                return was_full;
            }

            bool ws_session::queue::is_full() const {
                return items_.size() >= limit;
            }

            ws_session::queue::queue(ws_session &self) : self_(self) {
                static_assert(limit > 0, "queue_vm limit must be positive");
                items_.reserve(limit);
            }

        }
    }
}