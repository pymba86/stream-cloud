
#include <websocket.hpp>
#include <memory>
#include "context.hpp"
#include "ws_server.hpp"
#include "ws_listener.hpp"
#include <metadata.hpp>

namespace stream_cloud {
    namespace providers {
        namespace ws_server {
            class ws_server::impl final {
            public:
                impl() = default;

                ~impl() = default;

                impl &operator=(impl &&) = default;

                impl(impl &&) = default;

                impl &operator=(const impl &) = default;

                impl(const impl &) = default;

                std::shared_ptr<ws_listener> listener_;

            };

            ws_server::ws_server(config::config_context_t *ctx, actor::actor_address address,
                                 std::initializer_list<actor::actor_address> pipe) : data_provider(ctx, "ws"),
                                                                                     pimpl(new impl) {

                auto string_ip = ctx->config()["ws-ip"].as<std::string>();
                boost::asio::ip::address address_ = boost::asio::ip::make_address(string_ip);

                auto string_port = ctx->config()["ws-port"].as<std::string>();
                auto tmp_port = std::stoul(string_port);

                auto port = static_cast<unsigned short>(tmp_port);
                pimpl->listener_ = std::make_shared<ws_listener>(ctx->main_loop(), tcp::endpoint{address_, port},
                                                                 address, pipe);

                attach(
                        behavior::make_handler(
                                "write",
                                [this](behavior::context &ctx) -> void {
                                    auto t = ctx.message().body<api::transport>();
                                    std::unique_ptr<api::web_socket> transport(
                                            static_cast<api::web_socket *>(t.release()));
                                    pimpl->listener_->write(std::move(transport));
                                }
                        )
                );

                attach(
                        behavior::make_handler(
                                "close",
                                [this](behavior::context &ctx) -> void {
                                    auto t = ctx.message().body<api::transport>();
                                    std::unique_ptr<api::web_socket> transport(
                                            static_cast<api::web_socket *>(t.release()));
                                    pimpl->listener_->close(std::move(transport));
                                }
                        )
                );

                attach(
                        behavior::make_handler(
                                "remove",
                                [this](behavior::context &ctx) -> void {
                                    auto t = ctx.message().body<api::transport>();
                                    std::unique_ptr<api::web_socket> transport(
                                            static_cast<api::web_socket *>(t.release()));
                                    pimpl->listener_->remove(std::move(transport));
                                }
                        )
                );

            }

            ws_server::~ws_server() = default;

            void ws_server::startup(config::config_context_t *ctx) {
                pimpl->listener_->run();

            }

            void ws_server::shutdown() {
                pimpl->listener_->close_all();
            }

        }
    }
}