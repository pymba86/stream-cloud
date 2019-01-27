
#include "profile.hpp"
#include <sqlite_modern_cpp.h>
#include "task.hpp"
#include "websocket.hpp"
#include <unordered_set>


namespace stream_cloud {

    namespace platform {

        using namespace sqlite;

        class profile::impl final {
        public:
            impl(sqlite::database &db) : db_(db) {};

            ~impl() = default;

            bool is_auth(const std::string &profile_key) const {
                return profiles_auth.find(profile_key) != profiles_auth.end();
            }

            void add_auth(const std::string &profile_key) {
                profiles_auth.emplace(profile_key);
            }

            void remove_auth(const std::string &profile_key) {
                profiles_auth.erase(profile_key);
            }

            // Список профилей, которые прошли аунтетификацию
            // Необходим для меньших запросов к базе данных (кэш)
            std::unordered_set<std::string> profiles_auth;
            sqlite::database db_;
        };

        profile::profile(config::config_context_t *ctx) : abstract_service(ctx, "profile") {


            attach(
                    behavior::make_handler("signup", [this](behavior::context &ctx) -> void {
                        // Добавляем профиль

                        auto &task = ctx.message()->body<api::task>();

                        auto login = task.request.params["login"].as<std::string>();
                        auto password = task.request.params["password"].as<std::string>();

                        // Отправляем ответ
                        auto ws_response = new api::web_socket(task.transport_id_);
                        api::json_rpc::response_message response_message;
                        response_message.id = task.request.id;

                        if (!login.empty() && !password.empty()) {

                            try {
                                pimpl->db_ << "insert into profile (login,password) values (?,?);"
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
                    behavior::make_handler("login", [this](behavior::context &ctx) -> void {
                        // Получаем profile-key передач логин и пароль

                        auto &task = ctx.message()->body<api::task>();

                        auto login_param = task.request.params["login"].as<std::string>();
                        auto password_param = task.request.params["password"].as<std::string>();

                        // Отправляем ответ
                        auto ws_response = new api::web_socket(task.transport_id_);
                        api::json_rpc::response_message response_message;
                        response_message.id = task.request.id;

                        if (!login_param.empty() && !password_param.empty()) {

                            std::string login, password;

                            try {
                                pimpl->db_ << "select login, password from profile where login = ? limit 1;"
                                           << login_param
                                           >> [&](unique_ptr<string> login_p, unique_ptr<string> password_p) {
                                               login = login_p ? *login_p : string();
                                               password = password_p ? *password_p : string();
                                           };

                                if (password_param == password) {
                                    response_message.result = login + "." + password;
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
                        // Проверка профиля по базе

                        auto &task = ctx.message()->body<api::task>();

                        auto profile_key = task.request.metadata["profile-key"].as<std::string>();

                        std::vector<std::string> profile;
                        boost::algorithm::split(profile, profile_key, boost::is_any_of("."));

                        if (pimpl->is_auth(profile_key)) {

                            task.storage.emplace("profile.login", profile[0]);

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
                                pimpl->db_ << "select login, password from profile where login = ? limit 1;"
                                           << profile[0]
                                           >> [&](unique_ptr<string> login_p, unique_ptr<string> password_p) {
                                               login = login_p ? *login_p : string();
                                               password = password_p ? *password_p : string();
                                           };

                                if (profile[1] == password) {
                                    // Если пользователь зарегистрирован

                                    pimpl->add_auth(profile_key);

                                    task.storage.emplace("profile.login", profile[0]);

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

        void profile::startup(config::config_context_t *) {

            sqlite_config storage_config;
            storage_config.flags = OpenFlags::READWRITE | OpenFlags::CREATE | OpenFlags::FULLMUTEX;

            database db("platform_db", storage_config);

            db <<
               "create table if not exists profile ("
               "   login text primary key not null,"
               "   password text not null"
               ");";

            pimpl = std::make_unique<impl>(db);
        }

        void profile::shutdown() {

        }
    }
}