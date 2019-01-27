
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
#include "error.hpp"

namespace stream_cloud {
    namespace client {
        namespace ws_client {
            constexpr const char *dispatcher = "dispatcher";
            constexpr const char *handshake = "handshake";
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

            void ws_session::on_read(boost::system::error_code ec, std::size_t bytes_transferred) {
                boost::ignore_unused(bytes_transferred);

                if (ec) {
                    auto *error_m = new api::error(id_, api::transport_type::ws);
                    error_m->code = ec;
                    api::transport ws_error(error_m);
                    pipe_->send(
                            messaging::make_message(
                                    pipe_,
                                    error,
                                    api::transport(ws_error)
                            )
                    );
                    return;
                }

                auto *ws = new api::web_socket(id_);
                ws->body = boost::beast::buffers_to_string(buffer_.data());
                api::transport ws_data(ws);
                pipe_->send(
                        messaging::make_message(
                                pipe_,
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
                        ));
            }

            void ws_session::close() {
                boost::system::error_code ec;
                ws_.close(boost::beast::websocket::close_code::normal, ec);
            }

            void ws_session::run(const std::string &host, const std::string &port) {

                host_ = host;
                port_ = port;

                resolver_.async_resolve(
                        host,
                        port,
                        boost::asio::bind_executor(
                                strand_,
                                std::bind(
                                        &ws_session::on_resolve,
                                        shared_from_this(),
                                        std::placeholders::_1,
                                        std::placeholders::_2
                                )
                        ));
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
                        boost::asio::bind_executor(
                                strand_,
                                std::bind(
                                        &ws_session::on_connect,
                                        shared_from_this(),
                                        std::placeholders::_1
                                )
                        ));
            }

            void ws_session::on_connect(boost::system::error_code ec) {
                if (ec) {
                    auto *error_m = new api::error(id_, api::transport_type::ws);
                    error_m->code = ec;
                    api::transport ws_error(error_m);
                    pipe_->send(
                            messaging::make_message(
                                    pipe_,
                                    error,
                                    api::transport(ws_error)
                            )
                    );
                    return;
                }

                // Perform the websocket handshake
                ws_.async_handshake(host_, "/",
                                    boost::asio::bind_executor(
                                            strand_,
                                            std::bind(
                                                    &ws_session::on_handshake,
                                                    shared_from_this(),
                                                    std::placeholders::_1
                                            )
                                    ));
            }

            void ws_session::on_handshake(boost::system::error_code ec) {
                if (ec) {
                    return fail(ec, "handshake");
                }

                ws_.text(ws_.got_text());

                auto *ws = new api::web_socket(id_);
                ws->body = boost::beast::buffers_to_string(buffer_.data());
                api::transport ws_data(ws);
                pipe_->send(
                        messaging::make_message(
                                pipe_,
                                handshake,
                                api::transport(ws_data)
                        )
                );

                buffer_.consume(buffer_.size());

                // If we aren't at the queue limit, try to pipeline another request

                do_read();

            }


            ws_session::ws_session(boost::asio::io_context &ioc,
                                   api::transport_id id,
                                   actor::actor_address pipe_) :
                    ws_(ioc),
                    resolver_(ioc),
                    id_(id),
                    pipe_(pipe_),
                    strand_(ioc.get_executor()) {
                // setup_stream(ws_);
            }


            void ws_session::write(const std::shared_ptr<api::web_socket> &ptr) {
                std::string str = ptr->body;
                write(std::move(str));
            }

            void ws_session::write(std::string const &&str_) {
                auto const pstr = std::make_shared<std::string const>(std::move(str_));
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