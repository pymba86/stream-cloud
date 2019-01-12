
#include "profile.hpp"
#include <sqlite_modern_cpp.h>
#include "task.hpp"
#include "websocket.hpp"


namespace stream_cloud {

    namespace platform {

        using namespace sqlite;

        class profile::impl final {
        public:
            impl(sqlite::database &db) : db_(db) {};

            ~impl() = default;

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

                            } catch (exception& e) {
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
                    })
            );

            attach(
                    behavior::make_handler("authentication", [this](behavior::context &ctx) -> void {
                        // Проверка профиля по базе

                        auto &task = ctx.message().body<api::task>();

                        auto profile_key = task.request.metadata["profile-key"].as<std::string>();

                        std::vector<std::string> profile;
                        boost::algorithm::split(profile, profile_key, boost::is_any_of("."));

                        // Отправляем ответ
                        auto ws_response = new api::web_socket(task.transport_id_);
                        ws_response->body = "";

                        if (!profile_key.empty()) {

                            std::string login, password;

                            try {
                                pimpl->db_ << "select login, password from profile where login = ? limit 1;"
                                           << profile.at(0)
                                           >> tie(login, password);

                                if (profile.at(1) == password) {
                                    // Если пользователь зарегистрирован

                                    std::vector<std::string> dispatcher;
                                    boost::algorithm::split(dispatcher, task.request.method, boost::is_any_of("."));

                                    // Не должно быть зацикливания
                                    if (dispatcher.at(0) != "profile") {
                                        ctx->addresses(dispatcher.at(0))->send(
                                                messaging::make_message(
                                                        ctx->self(),
                                                        dispatcher.at(1),
                                                        std::move(task)
                                                )
                                        );
                                    }
                                    return;

                                } else {
                                    ws_response->body = "authentication close";
                                }

                            } catch (exception& e) {
                                ws_response->body = e.what();
                            }
                        } else {
                            ws_response->body = "profile_key empty";
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

        void profile::startup(config::config_context_t *) {

            sqlite_config storage_config;
            storage_config.flags = OpenFlags::READWRITE | OpenFlags::CREATE | OpenFlags::FULLMUTEX;

            database db("platform_db", storage_config);

            db <<
               "create table if not exists profile ("
               "   _id integer primary key autoincrement not null,"
               "   login text not null,"
               "   password text not null"
               ");";

            pimpl = std::make_unique<impl>(db);
        }

        void profile::shutdown() {

        }
    }
}