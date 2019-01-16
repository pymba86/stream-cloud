
#include <websocket.hpp>
#include "ws_session.hpp"

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>

namespace stream_cloud {
    namespace client {
        namespace ws_client {
            constexpr const char *dispatcher = "dispatcher";
            constexpr const char *handshake = "handshake";

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

            void ws_session::on_read(boost::system::error_code ec, std::size_t bytes_transferred) {
                boost::ignore_unused(bytes_transferred);

                if (ec == websocket::error::closed) {
                    return;
                }

                if (ec) {
                    return;
                }

                auto *ws = new api::web_socket(id_);
                ws->body = boost::beast::buffers_to_string(buffer_.data());
                pipe_->send(
                        messaging::make_message(
                                pipe_,
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
                        std::bind(
                                &ws_session::on_read,
                                shared_from_this(),
                                std::placeholders::_1,
                                std::placeholders::_2));
            }

            void ws_session::close()
            {
                boost::system::error_code ec;
                ws_.close(boost::beast::websocket::close_code::normal, ec);
            }

            void ws_session::run(const std::string &host, const std::string &port) {

                host_ = host;
                port_ = port;

                resolver_.async_resolve(
                        host,
                        port,
                        std::bind(
                                &ws_session::on_resolve,
                                shared_from_this(),
                                std::placeholders::_1,
                                std::placeholders::_2));
            }

            void ws_session::on_resolve(
                    boost::system::error_code ec,
                    tcp::resolver::results_type results) {
                if (ec)
                    return fail(ec, "resolve");

                // Make the connection on the IP address we get from a lookup
                boost::asio::async_connect(
                        ws_.next_layer(),
                        results.begin(),
                        results.end(),
                        std::bind(
                                &ws_session::on_connect,
                                shared_from_this(),
                                std::placeholders::_1));
            }

            void ws_session::on_connect(boost::system::error_code ec) {
                if (ec)
                    return fail(ec, "connect");

                // Perform the websocket handshake
                ws_.async_handshake(host_, "/",
                                    std::bind(
                                            &ws_session::on_handshake,
                                            shared_from_this(),
                                            std::placeholders::_1));
            }

            void ws_session::on_handshake(boost::system::error_code ec) {
                if (ec) {
                    return fail(ec, "handshake");
                }

                ws_.text(ws_.got_text());

                auto *ws = new api::web_socket(id_);
                ws->body = boost::beast::buffers_to_string(buffer_.data());
                pipe_->send(
                        messaging::make_message(
                                pipe_,
                                handshake,
                                api::transport(ws)
                        )
                );

                buffer_.consume(buffer_.size());

                // If we aren't at the queue limit, try to pipeline another request
                if (!queue_.is_full()) {
                    do_read();
                }
            }



            ws_session::ws_session(boost::asio::io_context &ioc,
                                   api::transport_id id,
                                   actor::actor_address pipe_) :
                    ws_(ioc),
                    resolver_(ioc),
                    id_(id),
                    pipe_(pipe_),
                    queue_(*this) {
                setup_stream(ws_);
            }


            void ws_session::write(const intrusive_ptr<api::web_socket> &ptr) {

                queue_(ptr->body);
            }

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