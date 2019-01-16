
#include "devices.hpp"
#include <sqlite_modern_cpp.h>
#include "websocket.hpp"
#include "task.hpp"
#include "json-rpc.hpp"

namespace stream_cloud {

    namespace manager {

        using namespace sqlite;

        class devices::impl final {
        public:
            impl(sqlite::database &db) : db_(db) {};

            ~impl() = default;

            sqlite::database db_;

        };

        devices::devices(config::config_context_t *ctx) : abstract_service(ctx, "devices") {


            attach(
                    behavior::make_handler("add", [this](behavior::context &ctx) -> void {
                        // Добавляем устройство

                        auto &task = ctx.message().body<api::task>();

                        auto name = task.request.params["name"].as<std::string>();
                        auto profile_login = task.storage["user.login"];

                        // Отправляем ответ
                        auto ws_response = new api::web_socket(task.transport_id_);
                        api::json_rpc::response_message response_message;
                        response_message.id = task.request.id;

                        if (!name.empty() && name.size() <= 6) {

                            std::string key = profile_login + "." + name;

                            try {
                                pimpl->db_ << "insert into devices (name, key) values (?,?);"
                                           << name
                                           << key;

                                response_message.result = api::json::json_value{
                                        {"key", key}
                                };

                            } catch (exception &e) {
                                response_message.error = api::json_rpc::response_error(
                                        api::json_rpc::error_code::unknown_error_code,
                                        e.what());
                            }
                        } else {
                            response_message.error = api::json_rpc::response_error(
                                    api::json_rpc::error_code::unknown_error_code,
                                    "name device empty or size > 6");
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
                        // Удаление устройства

                        auto &task = ctx.message().body<api::task>();

                        auto key = task.request.params["key"].as<std::string>();

                        // Отправляем ответ
                        auto ws_response = new api::web_socket(task.transport_id_);
                        api::json_rpc::response_message response_message;
                        response_message.id = task.request.id;

                        if (!key.empty()) {
                            try {
                                pimpl->db_ << "delete from devices where key = ?;"
                                           << key;

                                response_message.result = true;


                            } catch (exception &e) {
                                response_message.error = api::json_rpc::response_error(
                                        api::json_rpc::error_code::unknown_error_code,
                                        e.what());
                            }
                        } else {
                            response_message.error = api::json_rpc::response_error(
                                    api::json_rpc::error_code::unknown_error_code,
                                    "key device empty");
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
                        // Получить список устройств

                        auto &task = ctx.message().body<api::task>();

                        // Отправляем ответ
                        auto ws_response = new api::web_socket(task.transport_id_);
                        api::json_rpc::response_message response_message;
                        response_message.id = task.request.id;

                        try {
                            api::json::json_array managers_list;

                            pimpl->db_ << "select name, key, status from devices;"
                                       >> [&](std::string name, std::string key, std::string status) {

                                           api::json::json_map manager;

                                           manager["name"] = name;
                                           manager["key"] = key;
                                           manager["status"] = status;

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
        }

        void devices::startup(config::config_context_t *) {

            sqlite_config storage_config;
            storage_config.flags = OpenFlags::READWRITE | OpenFlags::CREATE | OpenFlags::FULLMUTEX;

            database db("manager_db", storage_config);

            db <<
               "create table if not exists devices ("
               "   key text primary key not null,"
               "   name text not null,"
               "   status integer not null default 0"
               ");";

            pimpl = std::make_unique<impl>(db);
        }

        void devices::shutdown() {

        }
    }
}