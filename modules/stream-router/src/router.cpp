
#include <router.hpp>

#include <unordered_map>
#include <unordered_set>

#include <transport_base.hpp>
#include <http.hpp>
#include <boost/format.hpp>
#include <intrusive_ptr.hpp>
#include <transport_base.hpp>
#include "websocket.hpp"

namespace stream_cloud {
    namespace router {
        class router::impl final {
        public:
            impl() = default;

            ~impl() = default;


        };

        router::router(config::config_context_t *ctx) :
                abstract_service(ctx, "router"),
                pimpl(std::make_unique<impl>()) {


            attach(
                    behavior::make_handler(
                            "dispatcher",
                            [this](behavior::context &ctx) -> void {
                                auto transport = ctx.message().body<api::transport>();
                                auto transport_type = transport->type();
                                std::string response = str(
                                        boost::format(R"({ "data": "%1%"})") % transport->id());


                                if (transport_type == api::transport_type::ws) {
                                    auto ws_response = new api::web_socket(transport->id());
                                    auto *ws = static_cast<api::web_socket *>(transport.get());

                                    ws_response->body = response;

                                    ctx->addresses("ws")->send(
                                            messaging::make_message(
                                                    ctx->self(),
                                                    "write",
                                                    api::transport(ws_response)
                                            )
                                    );
                                    return;
                                }

                                auto http_response = new api::http(transport->id());
                                auto *http = static_cast<api::http *>(transport.get());


                                if (transport_type == api::transport_type::http && http->uri() == "/system") {

                                    http_response->header("Content-Type", "application/json");
                                    http_response->body(response);

                                    ctx->addresses("http")->send(
                                            messaging::make_message(
                                                    ctx->self(),
                                                    "write",
                                                    api::transport(http_response)
                                            )
                                    );
                                } else if (transport_type == api::transport_type::http) {

                                    ctx->addresses("http")->send(
                                            messaging::make_message(
                                                    ctx->self(),
                                                    "close",
                                                    api::transport(http_response)
                                            )
                                    );
                                }


                            }
                    )
            );

        }

        router::~router() {

        };

        void router::startup(config::config_context_t *) {
        }

        void router::shutdown() {
        }
    }
}