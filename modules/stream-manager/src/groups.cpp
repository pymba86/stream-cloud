
#include "groups.hpp"
#include <sqlite_modern_cpp.h>
#include "task.hpp"
#include "websocket.hpp"
#include <unordered_set>

namespace stream_cloud {

    namespace manager {

        using namespace sqlite;

        class groups::impl final {
        public:
            impl(sqlite::database &db) : db_(db) {};

            ~impl() = default;

            sqlite::database db_;
        };

        groups::groups(config::config_context_t *ctx) : abstract_service(ctx, "groups") {


            attach(
                    behavior::make_handler("add", [this](behavior::context &ctx) -> void {
                        // Добавляет новую группу

                        auto &task = ctx.message()->body<api::task>();

                        auto key = task.request.params["key"].as<std::string>();
                        auto name = task.request.params["name"].as<std::string>();

                        // Отправляем ответ
                        auto ws_response = new api::web_socket(task.transport_id_);
                        api::json_rpc::response_message response_message;
                        response_message.id = task.request.id;

                        if (!key.empty() && !name.empty()) {

                            try {
                                pimpl->db_ << "insert into groups (key,name) values (?,?);"
                                           << key
                                           << name;

                                response_message.result = true;

                            } catch (std::exception &e) {
                                response_message.error = api::json_rpc::response_error(
                                        api::json_rpc::error_code::unknown_error_code,
                                        e.what());
                            }
                        } else {
                            response_message.error = api::json_rpc::response_error(
                                    api::json_rpc::error_code::unknown_error_code,
                                    "key or name empty");
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
                        // Удаляет группу

                        auto &task = ctx.message()->body<api::task>();

                        auto key = task.request.params["key"].as<std::string>();

                        // Отправляем ответ
                        auto ws_response = new api::web_socket(task.transport_id_);
                        api::json_rpc::response_message response_message;
                        response_message.id = task.request.id;

                        if (!key.empty()) {

                            try {
                                pimpl->db_ << "delete from groups where key = ?;"
                                           << key;

                                response_message.result = true;

                            } catch (std::exception &e) {
                                response_message.error = api::json_rpc::response_error(
                                        api::json_rpc::error_code::unknown_error_code,
                                        e.what());
                            }
                        } else {
                            response_message.error = api::json_rpc::response_error(
                                    api::json_rpc::error_code::unknown_error_code,
                                    "key group empty");
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
                        // Получить список пользователей

                        auto &task = ctx.message()->body<api::task>();

                        // Отправляем ответ
                        auto ws_response = new api::web_socket(task.transport_id_);
                        api::json_rpc::response_message response_message;
                        response_message.id = task.request.id;

                        try {
                            api::json::json_array users_list;

                            pimpl->db_ << "select key,name from groups;"
                                       >> [&](std::string key, std::string name) {
                                           users_list.push_back(api::json::json_map{
                                                   {"key",  key},
                                                   {"name", name}
                                           });
                                       };

                            response_message.result = users_list;

                        } catch (std::exception &e) {
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

        void groups::startup(config::config_context_t *) {

            sqlite_config storage_config;
            storage_config.flags = OpenFlags::READWRITE | OpenFlags::CREATE | OpenFlags::FULLMUTEX;

            database db("manager_db", storage_config);

            db <<
               "create table if not exists groups ("
               "   key text primary key not null,"
               "   name text unique not null"
               ");";

            pimpl = std::make_unique<impl>(db);
        }

        void groups::shutdown() {

        }
    }
}