
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
                // Inform the queue that a write completed
                _que.pop_front();

                if (!_que.empty()) {
                    ws_.async_write(
                            boost::asio::buffer(*_que.front()),
                            boost::asio::bind_executor(
                                    strand_,
                                    std::bind(
                                            &ws_session::on_write,
                                            shared_from_this(),
                                            std::placeholders::_1,
                                            std::placeholders::_2
                                    )
                            )
                    );
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

                    auto *error_m = new api::error(id_, api::transport_type::ws);
                    error_m->code = ec;
                    api::transport ws_error(error_m);
                    main_pipe_->send(
                            messaging::make_message(
                                    main_pipe_,
                                    error,
                                    api::transport(ws_error)
                            )
                    );

                    return;
                }

                auto ws = new api::web_socket(id_);
                ws->body = boost::beast::buffers_to_string(buffer_.data());

                api::transport ws_data(ws);
                main_pipe_->send(
                        messaging::make_message(
                                main_pipe_,
                                dispatcher,
                                api::transport(ws_data)
                        )
                );

                buffer_.consume(buffer_.size());

                // If we aren't at the queue limit, try to pipeline another request
                    do_read();

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
                    main_pipe_(main_pipe) {
                setup_stream(ws_);
            }

            void ws_session::write(const std::shared_ptr<api::web_socket> &ptr) {
                std::string str = ptr->body;
                write(std::move(str));
            }

            api::transport_id ws_session::id() const {
                return id_;
            }

            void ws_session::write(const std::string &value) {
                auto const pstr = std::make_shared<std::string const>(std::move(value));
                auto self = shared_from_this();
                boost::asio::post(ws_.get_executor(), boost::asio::bind_executor(strand_, [this, self, pstr] {
                    bool write_in_progress = !_que.empty();
                    _que.push_back(pstr);
                    if (!write_in_progress) {
                        ws_.async_write(
                                boost::asio::buffer(*_que.front()),
                                boost::asio::bind_executor(
                                        strand_,
                                        std::bind(
                                                &ws_session::on_write,
                                                shared_from_this(),
                                                std::placeholders::_1,
                                                std::placeholders::_2
                                        )
                                )
                        );
                    }
                }));
            };

        }
    }
}