
#include "subscriptions.hpp"
#include <sqlite_modern_cpp.h>
#include "task.hpp"
#include "websocket.hpp"
#include <unordered_set>

namespace stream_cloud {

    namespace manager {

        using namespace sqlite;

        class subscriptions::impl final {
        public:
            impl(sqlite::database &db) : db_(db) {};

            ~impl() = default;

            sqlite::database db_;
        };

        subscriptions::subscriptions(config::config_context_t *ctx) : abstract_service(ctx, "subscriptions") {


            attach(
                    behavior::make_handler("add", [this](behavior::context &ctx) -> void {
                        // Добавляет подписку на группу

                        auto &task = ctx.message().body<api::task>();

                        auto group_key = task.request.params["group-key"].as<std::string>();
                        auto user_login = task.request.params["user-login"].as<std::string>();

                        // Отправляем ответ
                        auto ws_response = new api::web_socket(task.transport_id_);
                        api::json_rpc::response_message response_message;
                        response_message.id = task.request.id;

                        if (!group_key.empty() && !user_login.empty()) {

                            try {
                                pimpl->db_ << "insert into subscriptions (group_key,user_login) values (?,?);"
                                           << group_key
                                           << user_login;

                                response_message.result = true;

                            } catch (exception &e) {
                                response_message.error = api::json_rpc::response_error(
                                        api::json_rpc::error_code::unknown_error_code,
                                        e.what());
                            }
                        } else {
                            response_message.error = api::json_rpc::response_error(
                                    api::json_rpc::error_code::unknown_error_code,
                                    "key or login empty");
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
                        // Удаляет подписку на группу

                        auto &task = ctx.message().body<api::task>();

                        auto group_key = task.request.params["group-key"].as<std::string>();
                        auto user_login = task.request.params["user-login"].as<std::string>();

                        // Отправляем ответ
                        auto ws_response = new api::web_socket(task.transport_id_);
                        api::json_rpc::response_message response_message;
                        response_message.id = task.request.id;

                        if (!group_key.empty() && !user_login.empty()) {

                            try {
                                pimpl->db_ << "delete from subscriptions where group_key = ? and user_login = ?;"
                                           << group_key
                                           << user_login;

                                response_message.result = true;

                            } catch (exception &e) {
                                response_message.error = api::json_rpc::response_error(
                                        api::json_rpc::error_code::unknown_error_code,
                                        e.what());
                            }
                        } else {
                            response_message.error = api::json_rpc::response_error(
                                    api::json_rpc::error_code::unknown_error_code,
                                    "key or login empty");
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
                        // Получить всех подписчиков

                        auto &task = ctx.message().body<api::task>();

                        auto group_key = task.request.params["group-key"].as<std::string>();

                        // Отправляем ответ
                        auto ws_response = new api::web_socket(task.transport_id_);
                        api::json_rpc::response_message response_message;
                        response_message.id = task.request.id;

                        if (!group_key.empty()) {

                            try {
                                api::json::json_array users_list;

                                pimpl->db_ << "select user_login from subscriptions where group_key = ?;"
                                           << group_key
                                           >> [&](std::string user_login) {
                                               users_list.push_back(user_login);
                                           };

                                response_message.result = users_list;

                            } catch (exception &e) {
                                response_message.error = api::json_rpc::response_error(
                                        api::json_rpc::error_code::unknown_error_code,
                                        e.what());
                            }
                        } else {
                            response_message.error = api::json_rpc::response_error(
                                    api::json_rpc::error_code::unknown_error_code,
                                    "key empty");
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
                    behavior::make_handler("devices", [this](behavior::context &ctx) -> void {
                        // Получить список устройств к которым подписан авторизованный пользователь

                        auto &task = ctx.message().body<api::task>();

                        auto user_login = task.storage["user.login"];

                        try {

                            api::json::json_array groups_list;

                            pimpl->db_ << "select group_key from subscriptions where user_login = ?;"
                                       << user_login
                                       >> [&](std::string group_key) {
                                           groups_list.push_back(group_key);
                                       };

                            task.request.params = groups_list;

                            ctx->addresses("connections")->send(
                                    messaging::make_message(
                                            ctx->self(),
                                            "groups",
                                            std::move(task)
                                    )
                            );
                        } catch (exception &e) {

                            // Отправляем ошибку
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
        }

        void subscriptions::startup(config::config_context_t *) {

            sqlite_config storage_config;
            storage_config.flags = OpenFlags::READWRITE | OpenFlags::CREATE | OpenFlags::FULLMUTEX;

            database db("manager_db", storage_config);

            db <<
               "create table if not exists subscriptions ("
               "   group_key text not null,"
               "   user_login text not null"
               ");";

            pimpl = std::make_unique<impl>(db);
        }

        void subscriptions::shutdown() {

        }
    }
}