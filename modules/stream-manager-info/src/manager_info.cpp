
#include "manager_info.hpp"
#include "task.hpp"
#include "websocket.hpp"
#include <unordered_set>
#include "context.hpp"

namespace stream_cloud {

    namespace system {


        class manager_info::impl final {
        public:
            impl(std::string manager_key) : manager_key(std::move(manager_key)) {};

            ~impl() = default;

            const std::string manager_key;
        };

        manager_info::manager_info(config::config_context_t *ctx) : abstract_service(ctx, "manager") {


            attach(
                    behavior::make_handler("info", [this](behavior::context &ctx) -> void {
                        // Получить ключ менеджера
                        // По дефолту берется из конфигурации

                        auto &task = ctx.message().body<api::task>();

                        // Отправляем ответ
                        auto ws_response = new api::web_socket(task.transport_id_);
                        api::json_rpc::response_message response_message;
                        response_message.id = task.request.id;
                        response_message.result = pimpl->manager_key;

                        ws_response->body = api::json_rpc::serialize(response_message);

                        ctx->addresses("ws")->send(
                                messaging::make_message(
                                        ctx->self(),
                                        "write",
                                        api::transport(ws_response)
                                )
                        );

                    })
            );
        }

        void manager_info::startup(config::config_context_t *ctx) {

            std::string manager_key;

            if (api::json_rpc::contains(ctx->config(), "manager-key")) {
                manager_key = ctx->config()["manager-key"].as<std::string>();
            } else {
                manager_key = "";
            }

            pimpl = std::make_unique<impl>(manager_key);
        }

        void manager_info::shutdown() {

        }
    }
}