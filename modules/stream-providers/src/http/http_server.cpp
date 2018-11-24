#include "http_server.hpp"

#include <context.hpp>
#include <http/listener.hpp>
#include <boost/asio.hpp>

namespace stream_cloud {
    namespace providers {
        namespace http_server {

            class http_server::impl final {
            public:
                impl() = default;

                ~impl() = default;

                impl &operator=(impl &&) = default;

                impl(impl &&) = default;

                impl &operator=(const impl &) = default;

                impl(const impl &) = default;

                std::shared_ptr<listener> listener_;

            };

            http_server::http_server(config::config_context_t *ctx, actor::actor_address address):
                    data_provider(ctx,"http"),
                    pimpl(new impl) {

                auto const address_ = boost::asio::ip::make_address("127.0.0.1");

               // auto string_port = ctx->config().as_object()["http-port"].as_string();
                auto string_port = "8080";
                auto tmp_port = std::stoul(string_port);
                auto port = static_cast<unsigned short>(tmp_port);

                pimpl->listener_ = std::make_shared<listener>(ctx->main_loop(), tcp::endpoint{address_, port},address);

                attach(
                        behavior::make_handler(
                                "write",
                                [this](behavior::context& ctx) -> void {
                                    auto& t = ctx.message().body<api::transport>();
                                    auto* transport_tmp = t.detach();
                                    std::unique_ptr<api::http> transport(static_cast<api::http*>(transport_tmp));
                                    pimpl->listener_->write(std::move(transport));
                                }
                        )
                );

                attach(
                        behavior::make_handler(
                                "add_trusted_url",
                                [this](behavior::context& ctx) -> void {
                                    auto app_name =ctx.message().body<std::string>();
                                    pimpl->listener_->add_trusted_url(app_name);
                                }
                        )
                );

            }

            http_server::~http_server() {

            }

            void http_server::startup(config::config_context_t *ctx) {
                pimpl->listener_->run();
            }

            void http_server::shutdown() {

            }


        }
    }
}