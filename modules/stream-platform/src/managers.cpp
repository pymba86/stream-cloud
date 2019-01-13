
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

            ~impl() = default;

            sqlite::database db_;
        };

        managers::managers(config::config_context_t *ctx) : abstract_service(ctx, "managers") {


            attach(
                    behavior::make_handler("add", [this](behavior::context &ctx) -> void {
                        // add_trusted_url - http
                        // Добавляем название, ключ
                        // write - ws

                        auto &task = ctx.message().body<api::task>();

                        auto name = task.request.params["name"].as<std::string>();
                        auto profile_login = task.storage["profile.login"];


                        // Отправляем ответ
                        auto ws_response = new api::web_socket(task.transport_id_);

                        if (!name.empty() && name.size() <= 6) {

                            std::string key = profile_login + "." + name;

                            try {
                                pimpl->db_ << "insert into managers (profile_login,name, key) values (?,?, ?);"
                                           << profile_login
                                           << name
                                           << key;

                                std::string url = profile_login + "/" + key;

                                ws_response->body = "insert done key: " + key;

                                ctx->addresses("http")->send(
                                        messaging::make_message(
                                                ctx->self(),
                                                "add_trusted_url",
                                                std::move(url)
                                        )
                                );
                            } catch (exception &e) {
                                ws_response->body = e.what();
                            }
                        } else {
                            ws_response->body = "name manager empty or size > 6";
                        }

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
                        // remove_trusted_url - http
                        // disconnect - managers
                        // Удаляем менеджера
                        // write - ws

                        auto &task = ctx.message().body<api::task>();

                        auto key = task.request.params["key"].as<std::string>();
                        auto profile_login = task.storage["profile.login"];


                        // Отправляем ответ
                        auto ws_response = new api::web_socket(task.transport_id_);

                        if (!key.empty()) {

                            try {
                                pimpl->db_ << "delete from managers where key = ? and profile_login =  ?;"
                                           << key
                                           << profile_login;

                                std::string url = profile_login + "/" + key;

                                ws_response->body = "delete manager: " + url;

                                ctx->addresses("http")->send(
                                        messaging::make_message(
                                                ctx->self(),
                                                "remove_trusted_url",
                                                std::move(url)
                                        )
                                );

                                ctx->addresses("router")->send(
                                        messaging::make_message(
                                                ctx->self(),
                                                "unregister_manager",
                                                std::move(task)
                                        )
                                );

                            } catch (exception &e) {
                                ws_response->body = e.what();
                            }
                        } else {
                            ws_response->body = "name manager empty";
                        }

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
                        // write - ws

                        auto &task = ctx.message().body<api::task>();

                        auto profile_login = task.storage["profile.login"];

                        // Отправляем ответ
                        auto ws_response = new api::web_socket(task.transport_id_);


                        try {

                            api::json_rpc::response_message response_message;
                            response_message.id = task.request.id;

                            api::json::json_array managers_list;

                            pimpl->db_ << "select name, key, status from managers where profile_login = ?;"
                                       << profile_login
                                       >> [&](std::string name, std::string key, std::string status) {

                                           api::json::json_map manager;

                                           manager["name"] = name;
                                           manager["key"] = key;
                                           manager["status"] = status;

                                           managers_list.push_back(manager);
                                       };

                            response_message.result = managers_list;
                            ws_response->body = api::json_rpc::serialize(response_message);

                        } catch (exception &e) {
                            ws_response->body = e.what();
                        }

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
                        // Добавляем / Меняем статус в базе данных / Проверяет на наличие
                        // register_manager - router

                        auto &task = ctx.message().body<api::task>();

                        auto key = task.request.params["key"].as<std::string>();
                        auto profile_login = task.storage["profile.login"];

                        // Отправляем ответ
                        auto ws_response = new api::web_socket(task.transport_id_);

                        try {

                            // Проверяем на наличие
                            int count_managers_key = 0;
                            pimpl->db_ << "select count(*) from managers where key = ? and profile_login = ?"
                                       << key
                                       << profile_login
                                       >> count_managers_key;

                            if (count_managers_key > 0) {
                                pimpl->db_ << "update managers set status = 1 where key = ? and profile_login = ?;"
                                           << key
                                           << profile_login;

                                task.storage.emplace("manager.key", key);

                                ctx->addresses("router")->send(
                                        messaging::make_message(
                                                ctx->self(),
                                                "register_manager",
                                                std::move(task)
                                        )
                                );

                                ws_response->body = "connect manager: " + profile_login + "/" + key;
                            } else {
                                ws_response->body = "manager key not found: " + key;
                            }

                        } catch (exception &e) {
                            ws_response->body = e.what();
                        }

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
                        // Удаляем / Меняем статус в базе данных
                        // unregister_manager - router

                        auto &task = ctx.message().body<api::task>();

                        auto key = task.request.params["key"].as<std::string>();
                        auto profile_login = task.storage["profile.login"];

                        // Отправляем ответ
                        auto ws_response = new api::web_socket(task.transport_id_);

                        try {
                            // Проверяем на наличие
                            int count_managers_key = 0;
                            pimpl->db_ << "select count(*) from managers where key = ? and profile_login = ?"
                                       << key
                                       << profile_login
                                       >> count_managers_key;

                            if (count_managers_key > 0) {
                                pimpl->db_ << "update managers set status = 0 where key = ? and profile_login = ?;"
                                           << key
                                           << profile_login;

                                task.storage.emplace("manager.key", key);

                                ws_response->body = "disconnect manager: " + profile_login + "/" + key;

                                ctx->addresses("router")->send(
                                        messaging::make_message(
                                                ctx->self(),
                                                "unregister_manager",
                                                std::move(task)
                                        )
                                );
                            } else {
                                ws_response->body = "manager key not found: " + key;
                            }

                        } catch (exception &e) {
                            ws_response->body = e.what();
                        }

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
               "   _id integer primary key autoincrement not null,"
               "   profile_login text not null,"
               "   name text not null,"
               "   key text unique not null,"
               "   status integer not null default 0"
               ");";

            pimpl = std::make_unique<impl>(db);
        }

        void managers::shutdown() {

        }
    }
}