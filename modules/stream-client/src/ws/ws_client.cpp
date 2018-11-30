
#include <websocket.hpp>
#include <memory>
#include "context.hpp"
#include "ws_client.hpp"
#include "ws_session.hpp"
#include <metadata.hpp>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <chrono>

namespace stream_cloud {
    namespace client {
        namespace ws_client {

            using clock = std::chrono::steady_clock;

            class ws_client::impl final {
            public:
                impl() = default;

                ~impl() = default;

                impl &operator=(impl &&) = default;

                impl(impl &&) = default;

                impl &operator=(const impl &) = default;

                impl(const impl &) = default;

                std::shared_ptr<ws_session> session_;
            };

            ws_client::ws_client(config::config_context_t *ctx, actor::actor_address address):data_provider(ctx,"client"),pimpl(new impl) {



              //  api::transport_id id_= static_cast<api::transport_id>(std::chrono::duration_cast<std::chrono::microseconds>(clock::now().time_since_epoch()).count());

                pimpl->session_ = std::make_shared<ws_session>(ctx->main_loop(), 0, address);


                attach(
                        behavior::make_handler(
                                "write",
                                [this](behavior::context& ctx) -> void {
                                    auto t = ctx.message().body<api::transport>();
                                    std::unique_ptr<api::web_socket> transport(static_cast<api::web_socket*>(t.release()));
                                    pimpl->session_->write(std::move(transport));
                                }
                        )
                );


            }

            ws_client::~ws_client() = default;

            void ws_client::startup(config::config_context_t *ctx) {
                std::string address_ =  "127.0.0.1";
                auto string_port = "8081";
                pimpl->session_->run(address_, string_port);

            }

            void ws_client::shutdown() {

            }

        }
    }
}