
#include <router.hpp>

#include <unordered_map>
#include <unordered_set>

#include <transport_base.hpp>
#include <http.hpp>
#include <boost/format.hpp>
#include <intrusive_ptr.hpp>

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
                                auto http_response = new api::http(transport->id());
                                auto http_print = new api::http(transport->id());
                                auto *http = static_cast<api::http *>(transport.get());

                              ctx->addresses("http")->send(
                                        messaging::make_message(
                                                ctx->self(),
                                                "print",
                                                api::transport(http_print)
                                        )
                                );

                                if (http->uri() == "/system") {
                                    std::string response = str(boost::format(R"({ "data": "%1%"})") % http_response->id());
                                    http_response->body(response);
                                    http_response->header("Content-Type", "application/json");

                                    ctx->addresses("http")->send(
                                            messaging::make_message(
                                                    ctx->self(),
                                                    "write",
                                                    api::transport(http_response)
                                            )
                                    );
                                } else {
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