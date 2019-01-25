
#include "subscriptions.hpp"
#include <sqlite_modern_cpp.h>
#include "task.hpp"
#include "websocket.hpp"
#include <unordered_set>

#include <iostream>
#include <vector>
#include <numeric>
#include <string>
#include <functional>

namespace stream_cloud {

    namespace manager {

        using namespace sqlite;

        class subscriptions::impl final {
        public:
            impl(sqlite::database &db) : db_(db) {};

            ~impl() = default;

            bool is_reg_device(const std::string &device_key) const {
                return device_subscribe_transport_reg.find(device_key) != device_subscribe_transport_reg.end();
            }

           const std::unordered_set<api::transport_id>& get_device_subscribes(const std::string &device_key) {
                return device_subscribe_transport_reg.at(device_key);
            }

            bool is_reg_device_subscribe(const std::string &device_key, const api::transport_id &id) {
                return get_device_subscribes(device_key).count(id) > 0;
            }

            void add_reg_device(const std::string &device_key) {
                device_subscribe_transport_reg.emplace(device_key, std::unordered_set<api::transport_id>());
            }

            void remove_reg_device(const std::string &device_key) {
                device_subscribe_transport_reg.erase(device_key);
            }


            sqlite::database db_;

            // Список устройств с id транспорта пользователей - куда отправить сообщение
            // device_key -> user_transport_id[]
            std::unordered_map<std::string, std::unordered_set<api::transport_id>> device_subscribe_transport_reg;
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

            attach(
                    behavior::make_handler("groups", [this](behavior::context &ctx) -> void {
                        // Проверка принадлежности пользователя к группе

                        auto &task = ctx.message().body<api::task>();

                        auto device_groups = task.request.metadata["device-groups"].as<api::json::json_array>();
                        auto user_login = task.storage["user.login"];
                        auto user_admin = task.storage["user.admin"];

                        if (!device_groups.empty()) {

                            try {

                                // Число групп пользоватля с доступом к устройству
                                int access_group_users = 0;
                                std::vector<std::string> groups;

                                for (auto const &group_key : device_groups) {
                                    groups.emplace_back(group_key.as<std::string>());
                                }

                                pimpl->db_
                                        << "select count(*) from subscriptions where user_login = ? and group_key in (?);"
                                        << user_login
                                        << std::accumulate(std::begin(groups), std::end(groups),
                                                           string(),
                                                           [](string &ss, string &s) {
                                                               return ss.empty() ? s : ss + "," + s;
                                                           })
                                        >> access_group_users;

                                task.request.metadata.erase("device-groups");

                                if (access_group_users > 0 || user_admin == "1") {

                                    ctx->addresses("devices")->send(
                                            messaging::make_message(
                                                    ctx->self(),
                                                    "request",
                                                    std::move(task)
                                            )
                                    );

                                } else {

                                    auto ws_response = new api::web_socket(task.transport_id_);
                                    api::json_rpc::response_message response_message;
                                    response_message.id = task.request.id;

                                    response_message.error = api::json_rpc::response_error(
                                            api::json_rpc::error_code::unknown_error_code,
                                            "access denied to device");

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

                        } else {

                            auto ws_response = new api::web_socket(task.transport_id_);
                            api::json_rpc::response_message response_message;
                            response_message.id = task.request.id;

                            response_message.error = api::json_rpc::response_error(
                                    api::json_rpc::error_code::unknown_error_code,
                                    "device-groups empty");

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
                    behavior::make_handler("on", [this](behavior::context &ctx) -> void {
                        // Проверка на подключение
                        // Добавление устройства к пользователю

                        auto &task = ctx.message().body<api::task>();

                        auto device_groups = task.request.metadata["device-groups"].as<api::json::json_array>();
                        auto device_key = task.request.params["device-key"].as<std::string>();
                        auto user_login = task.storage["user.login"];
                        auto user_admin = task.storage["user.admin"];

                        auto ws_response = new api::web_socket(task.transport_id_);
                        api::json_rpc::response_message response_message;
                        response_message.id = task.request.id;

                        if (!device_groups.empty()) {

                            try {

                                std::vector<std::string> user_groups;
                                std::vector<std::string> groups;

                                for (auto const &group_key : device_groups) {
                                    groups.emplace_back(group_key.as<std::string>());
                                }

                                pimpl->db_
                                        << "select group_key from subscriptions where user_login = ? and group_key in (?);"
                                        << user_login
                                        << std::accumulate(std::begin(groups), std::end(groups),
                                                           string(),
                                                           [](string &ss, string &s) {
                                                               return ss.empty() ? s : ss + "," + s;
                                                           })
                                        >> [&](std::string group_key) {
                                            user_groups.push_back(group_key);
                                        };

                                task.request.metadata.erase("device-groups");

                                if (user_groups.empty() && user_admin == "0") {

                                    response_message.error = api::json_rpc::response_error(
                                            api::json_rpc::error_code::unknown_error_code,
                                            "access denied to device");

                                } else {

                                    // Создаем новый список для устройства
                                    if (!pimpl->is_reg_device(device_key)) {
                                        pimpl->add_reg_device(device_key);
                                    }

                                    if (pimpl->is_reg_device_subscribe(device_key, task.transport_id_)) {

                                        response_message.error = api::json_rpc::response_error(
                                                api::json_rpc::error_code::unknown_error_code,
                                                "subscriptions already");

                                    } else {

                                        // Вставляем нового слушателя для устройства
                                        pimpl->device_subscribe_transport_reg.at(device_key).emplace(task.transport_id_);

                                        response_message.result = true;
                                    }
                                }

                            } catch (exception &e) {
                                response_message.error = api::json_rpc::response_error(
                                        api::json_rpc::error_code::unknown_error_code,
                                        e.what());
                            }

                        } else {
                            response_message.error = api::json_rpc::response_error(
                                    api::json_rpc::error_code::unknown_error_code,
                                    "device-groups empty");
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
                    behavior::make_handler("off", [this](behavior::context &ctx) -> void {
                        // Проверка на подключение
                        // Удаление устройства у пользователя

                        auto &task = ctx.message().body<api::task>();

                        auto device_key = task.request.params["device-key"].as<std::string>();
                        auto user_login = task.storage["user.login"];

                        auto ws_response = new api::web_socket(task.transport_id_);
                        api::json_rpc::response_message response_message;
                        response_message.id = task.request.id;


                        // Создаем новый список для устройства
                        if (pimpl->is_reg_device(device_key)) {


                            if (pimpl->is_reg_device_subscribe(device_key, task.transport_id_)) {

                                // Удаляем слушателя от устройства
                                pimpl->device_subscribe_transport_reg.at(device_key).erase(task.transport_id_);

                                response_message.result = true;
                            } else {

                                response_message.error = api::json_rpc::response_error(
                                        api::json_rpc::error_code::unknown_error_code,
                                        "unsubscribe already");
                            }


                            response_message.result = true;

                        } else {
                            response_message.error = api::json_rpc::response_error(
                                    api::json_rpc::error_code::unknown_error_code,
                                    "device is not connect");
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
                    behavior::make_handler("emit", [this](behavior::context &ctx) -> void {
                        // Отправка уведомления пользователям от устройства
                        auto &transport = ctx.message().body<api::transport>();

                        auto *ws = static_cast<api::web_socket *>(transport.get());

                        api::json::json_map message{api::json::data{ws->body}};

                        auto device_key = message["metadata"]["device-key"].as<std::string>();

                        if (pimpl->is_reg_device(device_key)) {

                            for (const auto &subscribe: pimpl->device_subscribe_transport_reg.at(device_key)) {

                                message["metadata"]["transport"] = subscribe;

                                auto ws_notify = new api::web_socket(subscribe);
                                ws_notify->body = message.to_string();

                                api::transport ws_not(ws_notify);

                                ctx->addresses("ws")->send(
                                        messaging::make_message(
                                                ctx->self(),
                                                "write",
                                                std::move(ws_not)
                                        )
                                );
                            }
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