
#include "connections.hpp"
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

        class connections::impl final {
        public:
            impl(sqlite::database &db) : db_(db) {};

            ~impl() = default;

            sqlite::database db_;
        };

        connections::connections(config::config_context_t *ctx) : abstract_service(ctx, "connections") {


            attach(
                    behavior::make_handler("add", [this](behavior::context &ctx) -> void {
                        // Добавляет устройство к группе

                        auto &task = ctx.message().body<api::task>();

                        auto group_key = task.request.params["group-key"].as<std::string>();
                        auto device_key = task.request.params["device-key"].as<std::string>();

                        // Отправляем ответ
                        auto ws_response = new api::web_socket(task.transport_id_);
                        api::json_rpc::response_message response_message;
                        response_message.id = task.request.id;

                        if (!group_key.empty() && !device_key.empty()) {

                            try {
                                pimpl->db_ << "insert into connections (group_key,device_key) values (?,?);"
                                           << group_key
                                           << device_key;

                                response_message.result = true;

                            } catch (exception &e) {
                                response_message.error = api::json_rpc::response_error(
                                        api::json_rpc::error_code::unknown_error_code,
                                        e.what());
                            }
                        } else {
                            response_message.error = api::json_rpc::response_error(
                                    api::json_rpc::error_code::unknown_error_code,
                                    "group-key or device-key empty");
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
                        // Удаляет устройство из группы

                        auto &task = ctx.message().body<api::task>();

                        auto group_key = task.request.params["group-key"].as<std::string>();
                        auto device_key = task.request.params["device-key"].as<std::string>();

                        // Отправляем ответ
                        auto ws_response = new api::web_socket(task.transport_id_);
                        api::json_rpc::response_message response_message;
                        response_message.id = task.request.id;

                        if (!group_key.empty() && !device_key.empty()) {

                            try {
                                pimpl->db_ << "delete from connections where group_key = ? and device_key = ?;"
                                           << group_key
                                           << device_key;

                                response_message.result = true;

                            } catch (exception &e) {
                                response_message.error = api::json_rpc::response_error(
                                        api::json_rpc::error_code::unknown_error_code,
                                        e.what());
                            }
                        } else {
                            response_message.error = api::json_rpc::response_error(
                                    api::json_rpc::error_code::unknown_error_code,
                                    "group-key or device-key empty");
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
                        // Получить устройства группы

                        auto &task = ctx.message().body<api::task>();

                        auto group_key = task.request.params["group-key"].as<std::string>();

                        // Отправляем ответ
                        auto ws_response = new api::web_socket(task.transport_id_);
                        api::json_rpc::response_message response_message;
                        response_message.id = task.request.id;

                        if (!group_key.empty()) {

                            try {
                                api::json::json_array users_list;

                                pimpl->db_ << "select device_key from connections where group_key = ?;"
                                           << group_key
                                           >> [&](std::string device_key) {
                                               users_list.push_back(device_key);
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
                                    "group-key empty");
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
                    behavior::make_handler("groups", [this](behavior::context &ctx) -> void {
                        // Получить список устройств по списку групп

                        auto &task = ctx.message().body<api::task>();

                        auto group_key_list = task.request.params.as<api::json::json_array>();

                        // Отправляем ответ
                        auto ws_response = new api::web_socket(task.transport_id_);
                        api::json_rpc::response_message response_message;
                        response_message.id = task.request.id;

                        if (!group_key_list.empty()) {

                            try {

                                std::vector<std::string> groups;
                                api::json::json_array devices_list;

                                for (auto const &group_key : group_key_list) {
                                    groups.emplace_back(group_key.as<std::string>());
                                }

                                pimpl->db_ << "select device_key from connections where group_key in (?);"
                                           << std::accumulate(std::begin(groups), std::end(groups), string(),
                                                              [](string &ss, string &s) {
                                                                  return ss.empty() ? s : ss + "," + s;
                                                              })
                                           >> [&](unique_ptr<string> device_key_p) {
                                               if (device_key_p) {
                                                   devices_list.push_back(*device_key_p);
                                               }
                                           };


                                response_message.result = devices_list;

                            } catch (exception &e) {
                                response_message.error = api::json_rpc::response_error(
                                        api::json_rpc::error_code::unknown_error_code,
                                        e.what());
                            }
                        } else {
                            response_message.error = api::json_rpc::response_error(
                                    api::json_rpc::error_code::unknown_error_code,
                                    "devices not found");
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
                        // Получить список групп по device-key

                        auto &task = ctx.message().body<api::task>();

                        auto device_key = task.request.metadata["device-key"].as<std::string>();

                        auto ws_response = new api::web_socket(task.transport_id_);
                        api::json_rpc::response_message response_message;
                        response_message.id = task.request.id;


                        if (!device_key.empty()) {

                            try {
                                api::json::json_array groups_list;

                                pimpl->db_ << "select group_key from connections where device_key = ?;"
                                           << device_key
                                           >> [&](std::string group_key) {
                                               groups_list.push_back(group_key);
                                           };

                                task.request.metadata["device-groups"] = groups_list;

                                ctx->addresses("subscriptions")->send(
                                        messaging::make_message(
                                                ctx->self(),
                                                "groups",
                                                std::move(task)
                                        )
                                );

                                return;

                            } catch (exception &e) {
                                response_message.error = api::json_rpc::response_error(
                                        api::json_rpc::error_code::unknown_error_code,
                                        e.what());
                            }
                        } else {
                            response_message.error = api::json_rpc::response_error(
                                    api::json_rpc::error_code::unknown_error_code,
                                    "device-key empty");
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

        void connections::startup(config::config_context_t *) {

            sqlite_config storage_config;
            storage_config.flags = OpenFlags::READWRITE | OpenFlags::CREATE | OpenFlags::FULLMUTEX;

            database db("manager_db", storage_config);

            db <<
               "create table if not exists connections ("
               "   group_key text not null,"
               "   device_key text not null"
               ");";

            pimpl = std::make_unique<impl>(db);
        }

        void connections::shutdown() {

        }
    }
}