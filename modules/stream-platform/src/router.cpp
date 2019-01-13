#include <utility>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/cxx11/any_of.hpp>

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
    namespace platform {

        class router::impl final {

        public:
            impl() = default;

            bool is_reg_manager(const std::string &manager_key) const {
                return managers_reg.find(manager_key) != managers_reg.end();
            }

            bool is_reg_manager(const api::transport_id id) const {

                auto it = std::find_if(std::begin(managers_reg), std::end(managers_reg), [&](auto &&p) {
                    return p.second == id;
                });

                return it != std::end(managers_reg);
            }

            void add_reg_manager(const std::string &manager_key, api::transport_id transport_id) {
                managers_reg.emplace(manager_key, transport_id);
            }

            void remove_reg_manager(const std::string &manager_key) {
                managers_reg.erase(manager_key);
            }

            api::transport_id get_manager_transport_id(const std::string &key) const {
                return managers_reg.at(key);
            }

            std::vector<std::string> get_service_list() const {
                return {"profile", "managers", "settings"};
            }

            std::vector<std::string> get_quest_method_list() const {
                return {"signup", "login"};
            }

            ~impl() = default;

        private:
            // Список подключенных менеджеров
            // Необходим для меньших запросов к базе данных (кэш)
            std::unordered_map<std::string, api::transport_id> managers_reg;

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

                                        // parse request
                                        api::task task_;
                                        task_.transport_id_ = transport->id();
                                        api::json_rpc::parse(ws->body, task_.request);

                                        // get name service and method call
                                        std::vector<std::string> dispatcher;
                                        boost::algorithm::split(dispatcher, task_.request.method,
                                                                boost::is_any_of("."));

                                        // check name service, method
                                        if (dispatcher.size() > 1) {

                                            task_.storage.emplace("service.name", dispatcher[0]);
                                            task_.storage.emplace("service.method", dispatcher[1]);

                                            if (api::json_rpc::contains(task_.request.metadata, "manager-key")) {

                                                auto manager_key = task_.request.metadata["manager-key"].get<std::string>();

                                                // Work in manager

                                                if (pimpl->is_reg_manager(manager_key)) {

                                                    auto manager_transport_id = pimpl->get_manager_transport_id(
                                                            manager_key);

                                                    api::json::json_map metadata;
                                                    metadata["transport"] = {transport->id()};

                                                    // send request to manager
                                                    auto ws_response = new api::web_socket(manager_transport_id);

                                                    api::json_rpc::notify_message notify_manager_message;
                                                    notify_manager_message.method = task_.request.method;
                                                    notify_manager_message.params = task_.request.params;
                                                    notify_manager_message.metadata = metadata;

                                                    ws_response->body = ws->body;

                                                    ctx->addresses("ws")->send(
                                                            messaging::make_message(
                                                                    ctx->self(),
                                                                    "write",
                                                                    api::transport(ws_response)
                                                            )
                                                    );

                                                } else {
                                                    // Менеджер не найден или не подключен
                                                }
                                            } else {

                                                // Work in platform

                                                auto service_name = dispatcher.at(0);

                                                // check white list service name
                                                if (boost::algorithm::any_of_equal(pimpl->get_service_list(),
                                                                                   service_name)) {

                                                    if (api::json_rpc::contains(task_.request.metadata,
                                                                                "profile-key")) {

                                                        ctx->addresses("profile")->send(
                                                                messaging::make_message(
                                                                        ctx->self(),
                                                                        "auth",
                                                                        std::move(task_)
                                                                )
                                                        );

                                                    } else {
                                                        // Переход на регистрацию
                                                        auto method_name = dispatcher.at(1);

                                                        // check white list service name
                                                        if (boost::algorithm::any_of_equal(
                                                                pimpl->get_quest_method_list(),
                                                                method_name)) {

                                                            ctx->addresses("profile")->send(
                                                                    messaging::make_message(
                                                                            ctx->self(),
                                                                            method_name,
                                                                            std::move(task_)
                                                                    )
                                                            );
                                                        } else {
                                                            auto ws_response = new api::web_socket(task_.transport_id_);
                                                            ws_response->body = "Метод не найден";
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
                                                    auto ws_response = new api::web_socket(task_.transport_id_);
                                                    ws_response->body = "Сервис не найден";
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
                                        }

                                    } else if (api::json_rpc::is_response(message)) {

                                        // Отправляем ответ от проверенного менеджера клиенту
                                        if (pimpl->is_reg_manager(transport->id())) {

                                            api::json_rpc::response_message response_message;
                                            api::json_rpc::parse(ws->body, response_message);

                                            auto transports_id = response_message.metadata["transport"].as<api::json::json_array>();

                                            // Убираем метаданные из ответа
                                            response_message.metadata.clear();

                                            // Формируем ответ
                                            std::string response = api::json_rpc::serialize(response_message);

                                            for (auto const &id : transports_id) {

                                                auto ws_response = new api::web_socket(id);
                                                ws_response->body = response;

                                                ctx->addresses("ws")->send(
                                                        messaging::make_message(
                                                                ctx->self(),
                                                                "write",
                                                                api::transport(ws_response)
                                                        )
                                                );
                                            }
                                        }
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

                        auto service = ctx->addresses(service_name);

                        if (service->message_types().count(method_name) && service_name != "profile") {
                            service->send(
                                    messaging::make_message(
                                            ctx->self(),
                                            method_name,
                                            std::move(task)
                                    ));
                        } else {
                            // send request to manager
                            auto ws_response = new api::web_socket(task.transport_id_);

                            api::json_rpc::response_message response_message;
                            response_message.id = task.request.id;
                            response_message.error = api::json_rpc::response_error(
                                    api::json_rpc::error_code::methodNot_found,
                                    "method not found:" + method_name);

                            ws_response->body =api::json_rpc::serialize(response_message);

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
                    behavior::make_handler("register_manager", [this](behavior::context &ctx) -> void {
                        // Добавляем менеджер когда он подключается.

                        auto &task = ctx.message().body<api::task>();
                        auto manager_key = task.storage["manager.key"];
                        pimpl->add_reg_manager(manager_key, task.transport_id_);
                    })
            );

            attach(
                    behavior::make_handler("unregister_manager", [this](behavior::context &ctx) -> void {
                        // Удаляем менеджер когда он отключается

                        auto &task = ctx.message().body<api::task>();
                        auto manager_key = task.storage["manager.key"];
                        pimpl->remove_reg_manager(manager_key);
                    })
            );

            attach(
                    behavior::make_handler("error", [this](behavior::context &ctx) -> void {
                        // Обработчка системных ошибок
                        auto &error = ctx.message().body<std::string>();
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