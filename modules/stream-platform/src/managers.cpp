
#include "managers.hpp"
#include <sqlite_modern_cpp.h>
#include "websocket.hpp"
#include "task.hpp"
#include "json-rpc.hpp"

namespace stream_cloud {

    namespace platform {

        using namespace sqlite;

        class managers::impl final {
        public:
            impl(sqlite::database &db) : db_(db) {};

            bool is_reg_manager(const std::string &manager_key) const {
                return managers_reg.find(manager_key) != managers_reg.end();
            }

            bool is_reg_manager_id(const api::transport_id id) const {

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

            ~impl() = default;

            sqlite::database db_;

        private:
            // Список подключенных менеджеров
            // Необходим для меньших запросов к базе данных (кэш)
            std::unordered_map<std::string, api::transport_id> managers_reg;
        };

        managers::managers(config::config_context_t *ctx) : abstract_service(ctx, "managers") {


            attach(
                    behavior::make_handler("add", [this](behavior::context &ctx) -> void {

                        auto &task = ctx.message().body<api::task>();

                        auto name = task.request.params["name"].as<std::string>();
                        auto profile_login = task.storage["profile.login"];


                        // Отправляем ответ
                        auto ws_response = new api::web_socket(task.transport_id_);
                        api::json_rpc::response_message response_message;
                        response_message.id = task.request.id;

                        if (!name.empty() && name.size() <= 6) {

                            std::string key = profile_login + "." + name;

                            try {
                                pimpl->db_ << "insert into managers (profile_login,name, key) values (?,?, ?);"
                                           << profile_login
                                           << name
                                           << key;

                                std::string url = profile_login + "/" + key;

                                response_message.result = api::json::json_value{
                                        {"key", key}
                                };

                                ctx->addresses("http")->send(
                                        messaging::make_message(
                                                ctx->self(),
                                                "add_trusted_url",
                                                std::move(url)
                                        )
                                );
                            } catch (exception &e) {
                                response_message.error = api::json_rpc::response_error(
                                        api::json_rpc::error_code::unknown_error_code,
                                        e.what());
                            }
                        } else {
                            response_message.error = api::json_rpc::response_error(
                                    api::json_rpc::error_code::unknown_error_code,
                                    "name manager empty or size > 6");
                        }

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

            attach(
                    behavior::make_handler("delete", [this](behavior::context &ctx) -> void {
                        // Удаление менеджера

                        auto &task = ctx.message().body<api::task>();

                        auto key = task.request.params["key"].as<std::string>();
                        auto profile_login = task.storage["profile.login"];


                        // Отправляем ответ
                        auto ws_response = new api::web_socket(task.transport_id_);
                        api::json_rpc::response_message response_message;
                        response_message.id = task.request.id;

                        if (!key.empty()) {

                            try {
                                pimpl->db_ << "delete from managers where key = ? and profile_login =  ?;"
                                           << key
                                           << profile_login;

                                std::string url = profile_login + "/" + key;

                                response_message.result = true;

                                ctx->addresses("http")->send(
                                        messaging::make_message(
                                                ctx->self(),
                                                "remove_trusted_url",
                                                std::move(url)
                                        )
                                );

                                if (pimpl->is_reg_manager(key)) {

                                    // Отправляем менеджеру сообщение об отключении
                                    auto ws_client_message = new api::web_socket(pimpl->get_manager_transport_id(key));
                                    api::json_rpc::request_message client_message;
                                    client_message.id = task.request.id;
                                    client_message.method = "disconnect";

                                    ws_client_message->body = api::json_rpc::serialize(client_message);

                                    ctx->addresses("ws")->send(
                                            messaging::make_message(
                                                    ctx->self(),
                                                    "write",
                                                    api::transport(ws_client_message)
                                            )
                                    );
                                }

                                pimpl->remove_reg_manager(key);

                            } catch (exception &e) {
                                response_message.error = api::json_rpc::response_error(
                                        api::json_rpc::error_code::unknown_error_code,
                                        e.what());
                            }
                        } else {
                            response_message.error = api::json_rpc::response_error(
                                    api::json_rpc::error_code::unknown_error_code,
                                    "key manager empty");
                        }

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

            attach(
                    behavior::make_handler("list", [this](behavior::context &ctx) -> void {
                        // Получить список менеджеров у пользователя

                        auto &task = ctx.message().body<api::task>();

                        auto profile_login = task.storage["profile.login"];

                        // Отправляем ответ
                        auto ws_response = new api::web_socket(task.transport_id_);
                        api::json_rpc::response_message response_message;
                        response_message.id = task.request.id;

                        try {
                            api::json::json_array managers_list;

                            pimpl->db_ << "select name, key from managers where profile_login = ?;"
                                       << profile_login
                                       >> [&](std::string name, std::string key, std::string status) {

                                           api::json::json_map manager;

                                           manager["name"] = name;
                                           manager["key"] = key;

                                           managers_list.push_back(manager);
                                       };

                            response_message.result = managers_list;

                        } catch (exception &e) {
                            response_message.error = api::json_rpc::response_error(
                                    api::json_rpc::error_code::unknown_error_code,
                                    e.what());
                        }

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

            attach(
                    behavior::make_handler("connect", [this](behavior::context &ctx) -> void {
                        // Подключаемя к платформе

                        auto &task = ctx.message().body<api::task>();

                        auto key = task.request.params["key"].as<std::string>();
                        auto profile_login = task.storage["profile.login"];

                        // Отправляем ответ
                        auto ws_response = new api::web_socket(task.transport_id_);
                        api::json_rpc::response_message response_message;
                        response_message.id = task.request.id;

                        if (pimpl->is_reg_manager(key)) {
                            response_message.error = api::json_rpc::response_error(
                                    api::json_rpc::error_code::unknown_error_code,
                                    "manager already connect");
                        } else {

                            try {

                                // Проверяем на наличие
                                int count_managers_key = 0;
                                pimpl->db_ << "select count(*) from managers where key = ? and profile_login = ?"
                                           << key
                                           << profile_login
                                           >> count_managers_key;

                                if (count_managers_key > 0) {

                                    response_message.result = true;

                                    pimpl->add_reg_manager(key, task.transport_id_);

                                } else {
                                    response_message.error = api::json_rpc::response_error(
                                            api::json_rpc::error_code::unknown_error_code,
                                            "manager not found");
                                }

                            } catch (exception &e) {
                                response_message.error = api::json_rpc::response_error(
                                        api::json_rpc::error_code::unknown_error_code,
                                        e.what());
                            }
                        }

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

            attach(
                    behavior::make_handler("disconnect", [this](behavior::context &ctx) -> void {
                        // Отключаемся от платформы

                        auto &task = ctx.message().body<api::task>();

                        auto key = task.request.params["key"].as<std::string>();
                        auto profile_login = task.storage["profile.login"];

                        try {
                            pimpl->remove_reg_manager(key);
                        } catch (exception &e) {

                            // Отправляем ответ
                            auto ws_response = new api::web_socket(task.transport_id_);
                            api::json_rpc::response_message response_message;
                            response_message.id = task.request.id;

                            response_message.error = api::json_rpc::response_error(
                                    api::json_rpc::error_code::unknown_error_code,
                                    e.what());

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
                    behavior::make_handler("request", [this](behavior::context &ctx) -> void {

                        auto &task = ctx.message().body<api::task>();

                        auto manager_key = task.request.metadata["manager-key"].get<std::string>();

                        if (pimpl->is_reg_manager(manager_key)) {

                            auto manager_transport_id = pimpl->get_manager_transport_id(manager_key);

                            task.request.metadata["transport"] = task.transport_id_;

                            // Отправляем сообщение менеджеру
                            auto ws_response = new api::web_socket(manager_transport_id);

                            api::json_rpc::request_message request_manager_message;
                            request_manager_message.method = task.request.method;
                            request_manager_message.params = task.request.params;
                            request_manager_message.metadata = task.request.metadata;

                            ws_response->body = api::json_rpc::serialize(request_manager_message);

                            ctx->addresses("ws")->send(
                                    messaging::make_message(
                                            ctx->self(),
                                            "write",
                                            api::transport(ws_response)
                                    )
                            );

                        } else {

                            // Отправляем сообщение отправителю
                            auto ws_response = new api::web_socket(task.transport_id_);
                            api::json_rpc::response_message response_message;
                            response_message.id = task.request.id;
                            response_message.error = api::json_rpc::response_error(
                                    api::json_rpc::error_code::unknown_error_code,
                                    "manager not connection");

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
                    behavior::make_handler("response", [this](behavior::context &ctx) -> void {

                        auto& transport = ctx.message().body<api::transport>();

                        auto *ws = static_cast<api::web_socket *>(transport.get());

                        // Отправляем ответ от проверенного менеджера клиенту
                        api::json::json_map message{api::json::data{ws->body}};

                        auto transports_id = message["metadata"]["transport"].as<api::transport_id>();

                        message.erase("metadata");

                        auto ws_response = new api::web_socket(transports_id);
                        ws_response->body = message.to_string();

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

        void managers::startup(config::config_context_t *) {

            sqlite_config storage_config;
            storage_config.flags = OpenFlags::READWRITE | OpenFlags::CREATE | OpenFlags::FULLMUTEX;

            database db("platform_db", storage_config);

            db <<
               "create table if not exists managers ("
               "   profile_login text not null,"
               "   name text not null,"
               "   key text primary key not null"
               ");";

            pimpl = std::make_unique<impl>(db);
        }

        void managers::shutdown() {

        }
    }
}