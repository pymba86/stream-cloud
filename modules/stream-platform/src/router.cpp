#include <utility>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/cxx11/any_of.hpp>

#include <router.hpp>

#include <unordered_map>
#include <unordered_set>

#include <transport_base.hpp>
#include <http.hpp>
#include <error.hpp>
#include <boost/format.hpp>
#include <intrusive_ptr.hpp>
#include <transport_base.hpp>
#include "websocket.hpp"
#include <thread>
#include <task.hpp>
#include <json-rpc.hpp>
#include <context.hpp>
#include <boost/utility/string_view.hpp>

namespace stream_cloud {
    namespace platform {

        class router::impl final {

        public:
            impl() = default;

            std::vector<std::string> get_service_list() const {
                return {"profile", "managers", "settings"};
            }

            std::vector<std::string> get_quest_method_list() const {
                return {"profile.signup", "profile.login"};
            }

            ~impl() = default;

        };

        router::router(config::config_context_t *ctx) :
                abstract_service(ctx, "router") {


            attach(
                    behavior::make_handler(
                            "dispatcher",
                            [this](behavior::context &ctx) -> void {

                                auto transport = ctx.message().body<api::transport>();
                                auto transport_type = transport->type();

                                if (transport_type == api::transport_type::ws) {

                                    auto *ws = static_cast<api::web_socket *>(transport.get());

                                    api::json::json_map message{api::json::data{ws->body}};

                                    if (api::json_rpc::is_request(message)) {

                                        api::task task_;
                                        task_.transport_id_ = ws->id();
                                        api::json_rpc::parse(message, task_.request);

                                        if (api::json_rpc::contains(task_.request.metadata, "manager-key")) {

                                            // Отправляем сообщение менеджеру
                                            ctx->addresses("managers")->send(
                                                    messaging::make_message(
                                                            ctx->self(),
                                                            "request",
                                                            std::move(task_)
                                                    )
                                            );
                                        } else {

                                            std::vector<std::string> dispatcher;
                                            boost::algorithm::split(dispatcher, task_.request.method,
                                                                    boost::is_any_of("."));

                                            if (dispatcher.size() > 1) {

                                                const auto service_name = dispatcher.at(0);
                                                const auto method_name = dispatcher.at(1);

                                                task_.storage.emplace("service.name", service_name);
                                                task_.storage.emplace("service.method", method_name);

                                                if (api::json_rpc::contains(task_.request.metadata,
                                                                            "profile-key")) {

                                                    // Проходим проверку
                                                    ctx->addresses("profile")->send(
                                                            messaging::make_message(
                                                                    ctx->self(),
                                                                    "auth",
                                                                    std::move(task_)
                                                            )
                                                    );

                                                } else {

                                                    if (boost::algorithm::any_of_equal(
                                                            pimpl->get_quest_method_list(),
                                                            task_.request.method)) {

                                                        ctx->addresses(service_name)->send(
                                                                messaging::make_message(
                                                                        ctx->self(),
                                                                        method_name,
                                                                        std::move(task_)
                                                                )
                                                        );
                                                    } else {

                                                        // Доступ запрещен
                                                        auto ws_response = new api::web_socket(task_.transport_id_);
                                                        api::json_rpc::response_message response_message;
                                                        response_message.id = task_.request.id;

                                                        response_message.error = api::json_rpc::response_error(
                                                                api::json_rpc::error_code::unknown_error_code,
                                                                "access is denied");

                                                        ws_response->body = api::json_rpc::serialize(response_message);

                                                        ctx->addresses("ws")->send(
                                                                messaging::make_message(
                                                                        ctx->self(),
                                                                        "write",
                                                                        api::transport(ws_response)
                                                                )
                                                        );

                                                    }
                                                }

                                            } else {

                                                // Не правильно указан метод
                                                auto ws_response = new api::web_socket(task_.transport_id_);
                                                api::json_rpc::response_message response_message;
                                                response_message.id = task_.request.id;

                                                response_message.error = api::json_rpc::response_error(
                                                        api::json_rpc::error_code::unknown_error_code,
                                                        "not specified correctly method");

                                                ws_response->body = api::json_rpc::serialize(response_message);

                                                ctx->addresses("ws")->send(
                                                        messaging::make_message(
                                                                ctx->self(),
                                                                "write",
                                                                api::transport(ws_response)
                                                        )
                                                );
                                            }
                                        }
                                    } else if (api::json_rpc::is_response(message)) {

                                        // Отправляем сообщение от менеджера
                                        ctx->addresses("managers")->send(
                                                messaging::make_message(
                                                        ctx->self(),
                                                        "response",
                                                        api::transport(ws)
                                                )
                                        );
                                    }
                                } else if (transport_type == api::transport_type::http) {
                                    // Проверяем на trusted_url
                                    // Отдаем клиента менеджера
                                    // Отдаем клиента платформы
                                    // Отдаем статичные файлы
                                } else {
                                    // Неопределенная ошибка
                                }
                            }
                    )
            );

            attach(
                    behavior::make_handler("service", [this](behavior::context &ctx) -> void {

                        auto &task = ctx.message().body<api::task>();

                        auto service_name = task.storage["service.name"];
                        auto method_name = task.storage["service.method"];


                        if (boost::algorithm::any_of_equal(pimpl->get_service_list(),
                                                           service_name)) {

                            auto service = ctx->addresses(service_name);

                            if (service->message_types().count(method_name)) {

                                // Вызываем метод из сервиса
                                service->send(
                                        messaging::make_message(
                                                ctx->self(),
                                                method_name,
                                                std::move(task)
                                        ));

                            } else {

                                auto ws_response = new api::web_socket(task.transport_id_);
                                api::json_rpc::response_message response_message;
                                response_message.id = task.request.id;

                                response_message.error = api::json_rpc::response_error(
                                        api::json_rpc::error_code::unknown_error_code,
                                        "method not found");

                                ws_response->body = api::json_rpc::serialize(response_message);

                                ctx->addresses("ws")->send(
                                        messaging::make_message(
                                                ctx->self(),
                                                "write",
                                                api::transport(ws_response)
                                        )
                                );
                            }
                        } else {

                            auto ws_response = new api::web_socket(task.transport_id_);
                            api::json_rpc::response_message response_message;
                            response_message.id = task.request.id;

                            response_message.error = api::json_rpc::response_error(
                                    api::json_rpc::error_code::unknown_error_code,
                                    "service not found");

                            ws_response->body = api::json_rpc::serialize(response_message);

                            ctx->addresses("ws")->send(
                                    messaging::make_message(
                                            ctx->self(),
                                            "write",
                                            api::transport(ws_response)
                                    )
                            );
                        }


                    })
            );

            attach(
                    behavior::make_handler("error", [this](behavior::context &ctx) -> void {
                        // Закрытие соединения

                        auto transport = ctx.message().body<api::transport>();
                        auto transport_type = transport->type();

                        if (transport_type == api::transport_type::ws) {

                            auto *error = static_cast<api::error *>(transport.get());

                            std::cout << error->code.message() << std::endl;

                            if (error->code == boost::asio::error::connection_reset
                                || error->code == boost::asio::error::not_connected) {
                                // Соединение сброшено на другой стороне

                                auto ws_response = new api::web_socket(error->id());

                                ctx->addresses("ws")->send(
                                        messaging::make_message(
                                                ctx->self(),
                                                "remove",
                                                api::transport(ws_response)
                                        )
                                );

                            } else {
                                auto ws_response = new api::web_socket(error->id());

                                ctx->addresses("ws")->send(
                                        messaging::make_message(
                                                ctx->self(),
                                                "close",
                                                api::transport(ws_response)
                                        )
                                );
                            }
                        }
                    })
            );

        }

        router::~router() {

        };

        void router::startup(config::config_context_t *) {
            pimpl = std::make_unique<impl>();
        }

        void router::shutdown() {
        }
    }
}