
#include "users.hpp"
#include <sqlite_modern_cpp.h>
#include "task.hpp"
#include "websocket.hpp"
#include <unordered_set>

namespace stream_cloud {

    namespace manager {

        using namespace sqlite;

        class users::impl final {
        public:
            impl(sqlite::database &db) : db_(db) {};

            ~impl() = default;

            bool is_auth(const std::string &user_key) const {
                return users_auth.find(user_key) != users_auth.end();
            }

            void add_auth(const std::string &user_key, const std::string &user_login) {
                users_auth.emplace(user_key, user_login);
            }

            bool is_auth_login(const std::string &user_login) const {

                auto it = std::find_if(std::begin(users_auth), std::end(users_auth), [&](auto &&p) {
                    return p.second == user_login;
                });

                return it != std::end(users_auth);
            }

            void remove_auth(const std::string &user_key) {
                users_auth.erase(user_key);
            }

            void remove_auth_login(const std::string &user_login) {
                auto it = std::find_if(std::begin(users_auth), std::end(users_auth), [&](auto &&p) {
                    return p.second == user_login;
                });

                if (it != std::end(users_auth)) {
                    users_auth.erase(it);
                }
            }

            // Список пользователей, которые прошли аунтетификацию
            // Необходим для меньших запросов к базе данных (кэш)
            // user_key -> user_login
            std::unordered_map<std::string, std::string> users_auth;
            sqlite::database db_;
        };

        users::users(config::config_context_t *ctx) : abstract_service(ctx, "users") {


            attach(
                    behavior::make_handler("add", [this](behavior::context &ctx) -> void {

                        auto &task = ctx.message().body<api::task>();

                        auto login = task.request.params["login"].as<std::string>();
                        auto password = task.request.params["password"].as<std::string>();

                        // Отправляем ответ
                        auto ws_response = new api::web_socket(task.transport_id_);
                        api::json_rpc::response_message response_message;
                        response_message.id = task.request.id;

                        if (!login.empty() || !password.empty()) {

                            try {
                                pimpl->db_ << "insert into users (login,password) values (?,?);"
                                           << login
                                           << password;

                                response_message.result = true;

                            } catch (exception &e) {
                                response_message.error = api::json_rpc::response_error(
                                        api::json_rpc::error_code::unknown_error_code,
                                        e.what());
                            }
                        } else {
                            response_message.error = api::json_rpc::response_error(
                                    api::json_rpc::error_code::unknown_error_code,
                                    "login or password empty");
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
                        // Удаляет пользователя

                        auto &task = ctx.message().body<api::task>();

                        auto login = task.request.params["login"].as<std::string>();

                        // Отправляем ответ
                        auto ws_response = new api::web_socket(task.transport_id_);
                        api::json_rpc::response_message response_message;
                        response_message.id = task.request.id;

                        if (!login.empty()) {

                            try {
                                pimpl->db_ << "delete from users where login = ?;"
                                           << login;

                                response_message.result = true;

                                pimpl->remove_auth_login(login);

                                ctx->addresses("groups")->send(
                                        messaging::make_message(
                                                ctx->self(),
                                                "remove_user",
                                                std::move(task)
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
                                    "name manager empty");
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

                        auto &task = ctx.message().body<api::task>();

                        auto profile_key = task.storage["profile.key"];

                        // Отправляем ответ
                        auto ws_response = new api::web_socket(task.transport_id_);
                        api::json_rpc::response_message response_message;
                        response_message.id = task.request.id;

                        try {
                            api::json::json_array users_list;

                            pimpl->db_ << "select login, password from users;"
                                       >> [&](std::string login, std::string password) {

                                           api::json::json_map manager;

                                           auto user_key = login + "." + password;
                                           auto admin_user = profile_key == user_key;

                                           users_list.push_back(api::json::json_map{
                                                   {"login", login},
                                                   {"admin", admin_user}
                                           });
                                       };

                            response_message.result = users_list;

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
                    behavior::make_handler("login", [this](behavior::context &ctx) -> void {
                        // Получаем user-key передав логин и пароль

                        auto &task = ctx.message().body<api::task>();

                        auto login_param = task.request.params["login"].as<std::string>();
                        auto password_param = task.request.params["password"].as<std::string>();
                        auto profile_key = task.storage["profile.key"];

                        // Отправляем ответ
                        auto ws_response = new api::web_socket(task.transport_id_);
                        api::json_rpc::response_message response_message;
                        response_message.id = task.request.id;

                        if (!login_param.empty() || !password_param.empty()) {

                            std::string login, password;

                            try {
                                pimpl->db_ << "select login, password from users where login = ? limit 1;"
                                           << login_param
                                           >> [&](unique_ptr<string> login_p, unique_ptr<string> password_p) {
                                               login = login_p ? *login_p : string();
                                               password = password_p ? *password_p : string();
                                           };

                                std::string user_key = login_param + "." + password_param;
                                auto admin_user = profile_key == user_key;

                                if (password_param == password || admin_user) {
                                    response_message.result = api::json::json_map{
                                            {"key", user_key},
                                            {"admin", admin_user}
                                    };
                                } else {
                                    response_message.error = api::json_rpc::response_error(
                                            api::json_rpc::error_code::unknown_error_code,
                                            "login or password is incorrect");
                                }
                            } catch (exception &e) {
                                response_message.error = api::json_rpc::response_error(
                                        api::json_rpc::error_code::unknown_error_code,
                                        e.what());
                            }
                        } else {
                            response_message.error = api::json_rpc::response_error(
                                    api::json_rpc::error_code::unknown_error_code,
                                    "login or password empty");
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
                    behavior::make_handler("auth", [this](behavior::context &ctx) -> void {
                        // Проверка пользователя по базе

                        auto &task = ctx.message().body<api::task>();

                        auto user_key = task.request.metadata["user-key"].as<std::string>();
                        auto profile_key = task.storage["profile.key"];

                        auto admin_user = profile_key == user_key;

                        task.storage["user.admin"] = std::to_string(admin_user);

                        std::vector<std::string> user;
                        boost::algorithm::split(user, user_key, boost::is_any_of("."));

                        if (pimpl->is_auth(user_key) || admin_user) {

                            task.storage.emplace("user.login", user[0]);

                            ctx->addresses("router")->send(
                                    messaging::make_message(
                                            ctx->self(),
                                            "service",
                                            std::move(task)
                                    )
                            );

                        } else {
                            std::string login, password;

                            try {
                                pimpl->db_ << "select login, password from users where login = ? limit 1;"
                                           << user[0]
                                           >> [&](unique_ptr<string> login_p, unique_ptr<string> password_p) {
                                               login = login_p ? *login_p : string();
                                               password = password_p ? *password_p : string();
                                           };;

                                if (user[1] == password) {
                                    // Если пользователь зарегистрирован

                                    pimpl->add_auth(user_key, login);

                                    task.storage.emplace("user.login", user[0]);

                                    ctx->addresses("router")->send(
                                            messaging::make_message(
                                                    ctx->self(),
                                                    "service",
                                                    std::move(task)
                                            )
                                    );

                                } else {
                                    auto ws_response = new api::web_socket(task.transport_id_);
                                    api::json_rpc::response_message response_message;
                                    response_message.id = task.request.id;

                                    response_message.error = api::json_rpc::response_error(
                                            api::json_rpc::error_code::unknown_error_code,
                                            "authentication bad");

                                    ws_response->body = api::json_rpc::serialize(response_message);

                                    ctx->addresses("ws")->send(
                                            messaging::make_message(
                                                    ctx->self(),
                                                    "write",
                                                    api::transport(ws_response)
                                            )
                                    );
                                }
                            } catch (exception &e) {
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
                        }
                    })
            );
        }

        void users::startup(config::config_context_t *) {

            sqlite_config storage_config;
            storage_config.flags = OpenFlags::READWRITE | OpenFlags::CREATE | OpenFlags::FULLMUTEX;

            database db("manager_db", storage_config);

            db <<
               "create table if not exists users ("
               "   login text primary key not null,"
               "   password text not null"
               ");";

            pimpl = std::make_unique<impl>(db);
        }

        void users::shutdown() {

        }
    }
}