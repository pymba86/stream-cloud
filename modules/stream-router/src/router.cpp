#include <utility>

#include <utility>
#include <router.hpp>

#include <unordered_map>
#include <unordered_set>

#include <transport_base.hpp>
#include <http.hpp>
#include <boost/format.hpp>
#include <intrusive_ptr.hpp>
#include <transport_base.hpp>
#include "websocket.hpp"
#include <thread>
#include <task.hpp>
#include <json-rpc.hpp>
#include <context.hpp>


namespace stream_cloud {
    namespace router {

        class router::impl final {
        public:
            impl() = default;

            ~impl() = default;
        };

        router::router(config::config_context_t *ctx) :
                abstract_service(ctx, "router") {

            attach(
                    behavior::make_handler(
                            "dispatcher",
                            [](behavior::context &ctx) -> void {
                                auto transport = ctx.message().body<api::transport>();
                                auto transport_type = transport->type();

                                


                                if (transport_type == api::transport_type::ws) {

                                    auto ws_response = new api::web_socket(transport->id());
                                    auto *ws = static_cast<api::web_socket *>(transport.get());
                                    api::task task_;
                                    api::json_rpc::parse(ws->body, task_.request);

                                    ctx->addresses("ws")->send(
                                            messaging::make_message(
                                                    ctx->self(),
                                                    "write",
                                                    api::transport(ws_response)
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