
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

            ws_server::ws_server(config::config_context_t *ctx, actor::actor_address address):data_provider(ctx,"ws"),pimpl(new impl) {

                boost::asio::ip::address address_ =  boost::asio::ip::make_address("127.0.0.1");

                auto string_port = "8081";
                auto tmp_port = std::stoul(string_port);

                auto port = static_cast<unsigned short>(tmp_port);

                pimpl->listener_ = std::make_shared<ws_listener>(ctx->main_loop(), tcp::endpoint{address_, port},address);

                attach(
                        behavior::make_handler(
                                "write",
                                [this](behavior::context& ctx) -> void {
                                    auto t = ctx.message().body<api::transport>();
                                    std::unique_ptr<api::web_socket> transport(static_cast<api::web_socket*>(t.release()));
                                    pimpl->listener_->write(std::move(transport));
                                }
                        )
                );


            }

            ws_server::~ws_server() = default;

            void ws_server::startup(config::config_context_t *ctx) {
                pimpl->listener_->run();

            }

            void ws_server::shutdown() {

            }

        }
    }
}