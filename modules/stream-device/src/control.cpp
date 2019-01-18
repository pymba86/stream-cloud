
#include "control.hpp"
#include "task.hpp"
#include "websocket.hpp"
#include <unordered_set>

namespace stream_cloud {

    namespace device {


        class control::impl final {
        public:
            impl()  {};

            ~impl() = default;

            bool status;
        };

        control::control(config::config_context_t *ctx) : abstract_service(ctx, "control") {


            attach(
                    behavior::make_handler("on", [this](behavior::context &ctx) -> void {
                        // Включает статус

                        auto &task = ctx.message().body<api::task>();

                        pimpl->status = true;

                        std::cout << "status: on" << std::endl;

                        // Отправляем ответ
                        auto ws_response = new api::web_socket(task.transport_id_);
                        api::json_rpc::response_message response_message;
                        response_message.id = task.request.id;

                        response_message.result = true;

                        ws_response->body = api::json_rpc::serialize(response_message);

                        ctx->addresses("manager")->send(
                                messaging::make_message(
                                        ctx->self(),
                                        "write",
                                        api::transport(ws_response)
                                )
                        );
                    })
            );

            attach(
                    behavior::make_handler("off", [this](behavior::context &ctx) -> void {
                        // Выключает статус

                        auto &task = ctx.message().body<api::task>();

                        pimpl->status = false;

                        std::cout << "status: false" << std::endl;

                        // Отправляем ответ
                        auto ws_response = new api::web_socket(task.transport_id_);
                        api::json_rpc::response_message response_message;
                        response_message.id = task.request.id;

                        response_message.result = true;

                        ws_response->body = api::json_rpc::serialize(response_message);

                        ctx->addresses("manager")->send(
                                messaging::make_message(
                                        ctx->self(),
                                        "write",
                                        api::transport(ws_response)
                                )
                        );

                    })
            );

            attach(
                    behavior::make_handler("status", [this](behavior::context &ctx) -> void {
                        // Получить значение переменной

                        auto &task = ctx.message().body<api::task>();

                        // Отправляем ответ
                        auto ws_response = new api::web_socket(task.transport_id_);
                        api::json_rpc::response_message response_message;
                        response_message.id = task.request.id;

                        response_message.result = pimpl->status;

                        ws_response->body = api::json_rpc::serialize(response_message);

                        ctx->addresses("manager")->send(
                                messaging::make_message(
                                        ctx->self(),
                                        "write",
                                        api::transport(ws_response)
                                )
                        );
                    })
            );
        }

        void control::startup(config::config_context_t *) {
            pimpl = std::make_unique<impl>();
        }

        void control::shutdown() {

        }
    }
}