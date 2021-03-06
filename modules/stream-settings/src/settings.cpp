
#include <settings.hpp>

#include <unordered_map>
#include <unordered_set>

#include <transport_base.hpp>
#include <http.hpp>
#include <boost/format.hpp>
#include <transport_base.hpp>
#include "websocket.hpp"
#include <thread>
#include <task.hpp>
#include <json-rpc.hpp>
#include <context.hpp>

namespace stream_cloud {
    namespace settings {
        class settings::impl final {
        public:
            impl(const api::json::json_map &config) : config_(config) {};

            ~impl() = default;

            api::json::json_map config_;
        };

        settings::settings(config::config_context_t *ctx) :
                abstract_service(ctx, "settings") {

            auto config = ctx->config();
            pimpl = std::make_unique<impl>(config);

            attach(
                    behavior::make_handler(
                            "settings",
                            [this](behavior::context &ctx) -> void {
                                auto transport = ctx.message()->body<api::transport>();
                                auto transport_type = transport->type();

                                if (transport_type == api::transport_type::ws) {

                                    auto ws_response = new api::web_socket(transport->id());
                                    auto *ws = static_cast<api::web_socket *>(transport.get());

                                    api::task task_;
                                    api::json_rpc::parse(ws->body, task_.request);

                                    if (task_.request.method == "get_settings") {
                                        api::json_rpc::response_message response(
                                                "2",
                                                pimpl->config_);

                                        ws_response->body = api::json_rpc::serialize(response);
                                    } else {
                                        api::json_rpc::response_message response("2","");
                                        api::json_rpc::response_error error(
                                                api::json_rpc::error_code::methodNot_found,
                                                "method not found");
                                        response.error = error;

                                        ws_response->body = api::json_rpc::serialize(response);
                                    }

                                    ctx->addresses("ws")->send(
                                            messaging::make_message(
                                                    ctx->self(),
                                                    "write",
                                                    api::transport(ws_response)
                                            )
                                    );

                                    return;
                                }


                            }
                    )
            );

        }

        settings::~settings() {

        };

        void settings::startup(config::config_context_t *) {
        }

        void settings::shutdown() {
        }
    }
}