
#include "ws_listener.hpp"
#include <websocket.hpp>

namespace stream_cloud {
    namespace providers {
        namespace ws_server {
            using clock = std::chrono::steady_clock;
            constexpr const char *write_handler_name = "write";

            ws_listener::ws_listener(boost::asio::io_context &ioc, tcp::endpoint endpoint,
                                     actor::actor_address main_pipe, std::initializer_list<actor::actor_address> pipe) :
                    strand_(ioc.get_executor()),
                    acceptor_(ioc),
                    socket_(ioc),
                    main_pipe_(main_pipe),
                    pipe_(pipe) {

                boost::system::error_code ec;

                acceptor_.open(endpoint.protocol(), ec);
                if (ec) {
                    fail(ec, "open");
                    return;
                }

                acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);

                if (ec) {
                    fail(ec, "set_option");
                    return;
                }

                acceptor_.bind(endpoint, ec);
                if (ec) {
                    fail(ec, "bind");
                    return;
                }

                acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);

                if (ec) {
                    fail(ec, "listen");
                    return;
                }
            }

            void ws_listener::run() {
                if (!acceptor_.is_open())
                    return;
                do_accept();
            }

            void ws_listener::do_accept() {
                acceptor_.async_accept(
                        socket_,
                        boost::asio::bind_executor(
                                strand_,
                                std::bind(
                                        &ws_listener::on_accept,
                                        shared_from_this(),
                                        std::placeholders::_1)));
            }

            void ws_listener::on_accept(boost::system::error_code ec) {

                if (ec) {
                    fail(ec, "accept");
                } else {
                    api::transport_id id_ = std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(
                            clock::now().time_since_epoch()).count());
                    auto session = std::make_shared<ws_session>(std::move(socket_), id_, main_pipe_);
                    storage_sessions.emplace(id_, std::move(session));
                    storage_sessions.at(id_)->run();

                }

                do_accept();
            }

            void ws_listener::write(const std::shared_ptr<api::web_socket> &ptr) {

                if (storage_sessions.count(ptr->id())) {
                    auto session = storage_sessions.at(ptr->id());
                    session->write(ptr);
                } else {

                    for (auto const &pipe : pipe_) {

                        api::transport transport(ptr);

                        pipe->send(
                                messaging::make_message(
                                        pipe,
                                        write_handler_name,
                                        api::transport(transport)
                                )
                        );
                    }
                }

            }

            void ws_listener::close(const std::shared_ptr<api::web_socket> &ptr) {

                if (storage_sessions.count(ptr->id())) {
                    auto session = storage_sessions.at(ptr->id());
                    session->close();
                    remove(ptr);
                }

            }

            void ws_listener::remove(const std::shared_ptr<api::web_socket> &ptr) {
                storage_sessions.erase(ptr->id());
            }

            void ws_listener::close_all() {
                for (const auto &session : storage_sessions) {
                    session.second->close();
                }
                storage_sessions.clear();
            };
        }
    }
}