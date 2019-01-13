
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

            bool is_auth_profile(const std::string &profile_key) const {
                return profiles_auth.find(profile_key) != profiles_auth.end();
            }

            void add_auth_profile(const std::string &profile_key) {
                profiles_auth.emplace(profile_key);
            }

            void remove_auth_profile(const std::string &profile_key) {
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

                        auto &task = ctx.message().body<api::task>();

                        auto login = task.request.params["login"].as<std::string>();
                        auto password = task.request.params["password"].as<std::string>();

                        // Отправляем ответ
                        auto ws_response = new api::web_socket(task.transport_id_);

                        if (!login.empty() || !password.empty()) {

                            try {
                                pimpl->db_ << "insert into profile (login,password) values (?,?);"
                                           << login
                                           << password;

                                ws_response->body = "insert done";

                            } catch (exception &e) {
                                ws_response->body = e.what();
                            }
                        } else {
                            ws_response->body = "login or password empty";
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
                    behavior::make_handler("login", [this](behavior::context &ctx) -> void {
                        // Получаем profile-key передач логин и пароль

                        auto &task = ctx.message().body<api::task>();

                        auto login_param = task.request.params["login"].as<std::string>();
                        auto password_param = task.request.params["password"].as<std::string>();

                        // Отправляем ответ
                        auto ws_response = new api::web_socket(task.transport_id_);

                        if (!login_param.empty() || !password_param.empty()) {

                            std::string login, password;

                            try {
                                pimpl->db_ << "select login, password from profile where login = ? limit 1;"
                                           << login_param
                                           >> tie(login, password);

                                if (password_param == password) {
                                    ws_response->body = login + "." + password;
                                } else {
                                    ws_response->body = "username or password is incorrect";
                                }
                            } catch (exception &e) {
                                ws_response->body = e.what();
                            }
                        } else {
                            ws_response->body = "login or password empty";
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
                    behavior::make_handler("auth", [this](behavior::context &ctx) -> void {
                        // Проверка профиля по базе

                        auto &task = ctx.message().body<api::task>();

                        auto profile_key = task.request.metadata["profile-key"].as<std::string>();

                        std::vector<std::string> profile;
                        boost::algorithm::split(profile, profile_key, boost::is_any_of("."));

                        if (pimpl->is_auth_profile(profile_key)) {

                            task.storage.emplace("profile.login", profile[0]);

                                ctx->addresses("router")->send(
                                        messaging::make_message(
                                                ctx->self(),
                                                "service",
                                                std::move(task)
                                        )
                                );
                            return;

                        } else {
                            std::string login, password;
                            // Отправляем ответ
                            auto ws_response = new api::web_socket(task.transport_id_);
                            ws_response->body = "";

                            try {
                                pimpl->db_ << "select login, password from profile where login = ? limit 1;"
                                           << profile[0]
                                           >> tie(login, password);

                                if (profile[1] == password) {
                                    // Если пользователь зарегистрирован

                                    pimpl->add_auth_profile(profile_key);

                                    task.storage.emplace("profile.login", profile[0]);

                                    ctx->addresses("router")->send(
                                            messaging::make_message(
                                                    ctx->self(),
                                                    "service",
                                                    std::move(task)
                                            )
                                    );
                                    return;

                                } else {
                                    ws_response->body = "authentication close";
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
               "   _id integer primary key autoincrement not null,"
               "   login text unique not null,"
               "   password text not null"
               ");";

            pimpl = std::make_unique<impl>(db);
        }

        void profile::shutdown() {

        }
    }
}