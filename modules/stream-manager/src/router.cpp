#include <utility>

#include <utility>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/cxx11/any_of.hpp>

#include "router.hpp"
#include <error.hpp>
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
    namespace manager {

        class router::impl final {

        public:
            impl(std::string profile_key, std::string manager_key)
                    : profile_key(std::move(profile_key)), manager_key(std::move(manager_key)) {};

            std::vector<std::string> get_service_list() const {
                return {"users", "devices", "settings", "groups", "subscriptions", "connections", "devices"};
            }

            std::vector<std::string> get_quest_method_list() const {
                return {"users.login"};
            }

            std::vector<std::string> get_user_method_list() const {
                return {"devices.list", "devices.control", "devices.detail"};
            }

            ~impl() = default;

            const std::string profile_key;
            const std::string manager_key;

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


                                        std::vector<std::string> dispatcher;
                                        boost::algorithm::split(dispatcher, task_.request.method,
                                                                boost::is_any_of("."));


                                        if (api::json_rpc::contains(task_.request.metadata,
                                                                    "manager-key")) {

                                            // Проверяем на соответствие ключей
                                            auto key = task_.request.metadata["manager-key"].as<std::string>();

                                            if (key != pimpl->manager_key) {

                                                auto ws_response = new api::web_socket(task_.transport_id_);
                                                api::json_rpc::response_message response_message;
                                                response_message.id = task_.request.id;
                                                response_message.metadata = task_.request.metadata;

                                                response_message.error = api::json_rpc::response_error(
                                                        api::json_rpc::error_code::unknown_error_code,
                                                        "manager key not equals");

                                                ws_response->body = api::json_rpc::serialize(response_message);

                                                ctx->addresses("ws")->send(
                                                        messaging::make_message(
                                                                ctx->self(),
                                                                "write",
                                                                api::transport(ws_response)
                                                        )
                                                );
                                                return;
                                            }
                                        } else {
                                            auto ws_response = new api::web_socket(task_.transport_id_);
                                            api::json_rpc::response_message response_message;
                                            response_message.id = task_.request.id;
                                            response_message.metadata = task_.request.metadata;

                                            response_message.error = api::json_rpc::response_error(
                                                    api::json_rpc::error_code::unknown_error_code,
                                                    "manager key required");

                                            ws_response->body = api::json_rpc::serialize(response_message);

                                            ctx->addresses("ws")->send(
                                                    messaging::make_message(
                                                            ctx->self(),
                                                            "write",
                                                            api::transport(ws_response)
                                                    )
                                            );
                                            return;
                                        }

                                        if (dispatcher.size() > 1) {

                                            const auto service_name = dispatcher.at(0);
                                            const auto method_name = dispatcher.at(1);

                                            task_.storage.emplace("service.name", service_name);
                                            task_.storage.emplace("service.method", method_name);
                                            task_.storage.emplace("profile.key", pimpl->profile_key);

                                            if (api::json_rpc::contains(task_.request.metadata,
                                                                        "user-key")) {

                                                // Проходим проверку
                                                ctx->addresses("users")->send(
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

                                    } else if (api::json_rpc::is_response(message)) {
                                        // Ответ от устройства
                                        std::cout << "device: " << api::json::json_map{api::json::data{ws->body}}
                                                  << std::endl;
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


                                if (task.storage["user.admin"] == "1") {
                                    service->send(
                                            messaging::make_message(
                                                    ctx->self(),
                                                    method_name,
                                                    std::move(task)
                                            ));
                                } else if (boost::algorithm::any_of_equal(
                                        pimpl->get_user_method_list(),
                                        task.request.method)) {

                                    ctx->addresses(service_name)->send(
                                            messaging::make_message(
                                                    ctx->self(),
                                                    method_name,
                                                    std::move(task)
                                            )
                                    );

                                } else {
                                    auto ws_response = new api::web_socket(task.transport_id_);
                                    api::json_rpc::response_message response_message;
                                    response_message.id = task.request.id;

                                    response_message.error = api::json_rpc::response_error(
                                            api::json_rpc::error_code::unknown_error_code,
                                            "permission denied");

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
                        // Обработка ошибок

                        auto transport = ctx.message().body<api::transport>();
                        auto transport_type = transport->type();

                        if (transport_type == api::transport_type::ws) {

                            auto *error = static_cast<api::error *>(transport.get());

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

        void router::startup(config::config_context_t *ctx) {

            auto profile_key = ctx->config()["profile-key"].as<std::string>();
            auto manager_key = ctx->config()["manager-key"].as<std::string>();

            pimpl = std::make_unique<impl>(profile_key, manager_key);
        }

        void router::shutdown() {
        }
    }
}