
#include "devices.hpp"
#include <sqlite_modern_cpp.h>
#include "websocket.hpp"
#include "task.hpp"
#include "json-rpc.hpp"
#include <unordered_set>

namespace stream_cloud {

    namespace manager {

        using namespace sqlite;

        class devices::impl final {
        public:
            impl(sqlite::database &db) : db_(db) {};

            ~impl() = default;

            bool is_reg_device(const std::string &device_key) const {
                return devices_reg.find(device_key) != devices_reg.end();
            }

            bool is_reg_device_id(const api::transport_id id) const {

                auto it = std::find_if(std::begin(devices_reg), std::end(devices_reg), [&](auto &&p) {
                    return p.second == id;
                });

                return it != std::end(devices_reg);
            }

            void add_reg_device(const std::string &device_key,
                                api::transport_id transport_id, std::unordered_set<std::string> actions,
                                std::unordered_set<std::string> variables) {
                devices_reg.emplace(device_key, transport_id);
                device_actions_reg.emplace(device_key, actions);
                device_variables_reg.emplace(device_key, variables);
            }

            void remove_reg_device(const std::string &device_key) {
                devices_reg.erase(device_key);
                device_actions_reg.erase(device_key);
                device_variables_reg.erase(device_key);
            }

            api::transport_id get_device_transport_id(const std::string &key) const {
                return devices_reg.at(key);
            }

            const std::unordered_set<std::string> &get_device_actions(const std::string &key) const {
                return device_actions_reg.at(key);
            }

            const std::unordered_set<std::string> &get_device_variables(const std::string &key) const {
                return device_variables_reg.at(key);
            }

            sqlite::database db_;

        private:
            // ???????????? ???????????????????????? ??????????????????
            // ?????????????????? ?????? ?????????????? ???????????????? ?? ???????? ???????????? (??????)
            std::unordered_map<std::string, api::transport_id> devices_reg;

            // ???????????? ???????????????????????????????????? ???????????????? ????????????????????
            // device_key -> device_actions
            std::unordered_map<std::string, std::unordered_set<std::string>> device_actions_reg;

            // ???????????? ???????????????????? ????????????????????
            // device_key -> device_variables
            std::unordered_map<std::string, std::unordered_set<std::string>> device_variables_reg;

        };

        devices::devices(config::config_context_t *ctx) : abstract_service(ctx, "devices") {


            attach(
                    behavior::make_handler("add", [this](behavior::context &ctx) -> void {
                        // ?????????????????? ????????????????????

                        auto &task = ctx.message()->body<api::task>();

                        auto name = task.request.params["name"].as<std::string>();
                        auto user_login = task.storage["user.login"];

                        // ???????????????????? ??????????
                        auto ws_response = new api::web_socket(task.transport_id_);
                        api::json_rpc::response_message response_message;
                        response_message.id = task.request.id;

                        if (!name.empty()) {

                            std::string key = user_login + "." + name;

                            try {
                                pimpl->db_ << "insert into devices (name, key) values (?,?);"
                                           << name
                                           << key;

                                response_message.result = key;

                            } catch (std::exception &e) {
                                response_message.error = api::json_rpc::response_error(
                                        api::json_rpc::error_code::unknown_error_code,
                                        e.what());
                            }
                        } else {
                            response_message.error = api::json_rpc::response_error(
                                    api::json_rpc::error_code::unknown_error_code,
                                    "name device empty");
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
                        // ???????????????? ????????????????????

                        auto &task = ctx.message()->body<api::task>();

                        auto key = task.request.params["key"].as<std::string>();

                        // ???????????????????? ??????????
                        auto ws_response = new api::web_socket(task.transport_id_);
                        api::json_rpc::response_message response_message;
                        response_message.id = task.request.id;

                        if (!key.empty()) {
                            try {
                                pimpl->db_ << "delete from devices where key = ?;"
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
                        // ???????????????? ???????????? ??????????????????

                        auto &task = ctx.message()->body<api::task>();

                        // ???????????????????? ??????????
                        auto ws_response = new api::web_socket(task.transport_id_);
                        api::json_rpc::response_message response_message;
                        response_message.id = task.request.id;

                        try {
                            api::json::json_array managers_list;

                            pimpl->db_ << "select name, key from devices;"
                                       >> [&](std::string name, std::string key, std::string status) {

                                           api::json::json_map manager;

                                           manager["name"] = name;
                                           manager["key"] = key;

                                           managers_list.push_back(manager);
                                       };

                            response_message.result = managers_list;

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

            attach(
                    behavior::make_handler("detail", [this](behavior::context &ctx) -> void {
                        // ???????????????? ???????????? ????????????????????

                        auto &task = ctx.message()->body<api::task>();

                        auto key = task.request.params["key"].as<std::string>();

                        // ???????????????????? ??????????
                        auto ws_response = new api::web_socket(task.transport_id_);
                        api::json_rpc::response_message response_message;
                        response_message.id = task.request.id;

                        if (pimpl->is_reg_device(key)) {

                            try {

                                api::json::json_map device_info;

                                pimpl->db_ << "select key, name from devices where key = ? limit 1;"
                                           << key
                                           >> [&](std::string key, std::string name) {

                                               device_info["name"] = name;
                                               device_info["key"] = key;

                                               const auto &device_actions = pimpl->get_device_actions(key);
                                               api::json::json_array result_actions;

                                               for (auto const &action : device_actions) {
                                                   result_actions.push_back(action);
                                               }

                                               const auto &device_variables = pimpl->get_device_variables(key);
                                               api::json::json_array result_variables;

                                               for (auto const &variable : device_variables) {
                                                   result_variables.push_back(variable);
                                               }

                                               device_info["actions"] = result_actions;
                                               device_info["variables"] = result_variables;

                                           };

                                response_message.result = device_info;

                            } catch (std::exception &e) {
                                response_message.error = api::json_rpc::response_error(
                                        api::json_rpc::error_code::unknown_error_code,
                                        e.what());
                            }
                        } else {
                            response_message.error = api::json_rpc::response_error(
                                    api::json_rpc::error_code::unknown_error_code,
                                    "device not connected");
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
                    behavior::make_handler("control", [this](behavior::context &ctx) -> void {
                        // ???????????? ?????????????????? action ?? ??????????????????
                        // ???????????????? ?????????????? ???? ????????????????????
                        // ???????????????? ???????????? ?????????? ???? device-key ?? connections
                        // ???? connections ?????????????????? ???????????? ?????????? ?? subscriptions
                        //  ???? ???????????????? ???????????????????????????? ???????? ?? ?????????? ????????????
                        // ?????????????????? ?????????????? ?? devices:request

                        auto &task = ctx.message()->body<api::task>();

                        if (api::json_rpc::contains(task.request.metadata, "device-key")) {

                            auto device_key = task.request.metadata["device-key"].as<std::string>();

                            if (pimpl->is_reg_device(device_key)) {

                                ctx->addresses("connections")->send(
                                        messaging::make_message(
                                                ctx->self(),
                                                "devices",
                                                std::move(task)
                                        )
                                );

                            } else {

                                auto ws_response = new api::web_socket(task.transport_id_);
                                api::json_rpc::response_message response_message;

                                response_message.id = task.request.id;
                                response_message.error = api::json_rpc::response_error(
                                        api::json_rpc::error_code::unknown_error_code,
                                        "device not connected");

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
                                    "device-key not found");

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
                    behavior::make_handler("connect", [this](behavior::context &ctx) -> void {
                        // ?????????????????????? ???????????????????? ?? ??????????????????

                        auto &task = ctx.message()->body<api::task>();

                        auto key = task.request.params["key"].as<std::string>();
                        auto actions = task.request.params["actions"].as<api::json::json_array>();
                        auto variables = task.request.params["variables"].as<api::json::json_array>();

                        // ???????????????????? ??????????
                        auto ws_response = new api::web_socket(task.transport_id_);
                        api::json_rpc::response_message response_message;
                        response_message.id = task.request.id;

                        try {

                            // ?????????????????? ???? ??????????????
                            int count_device_key = 0;
                            pimpl->db_ << "select count(*) from devices where key = ?;"
                                       << key
                                       >> count_device_key;

                            if (count_device_key > 0) {

                                if (!actions.empty() || !variables.empty()) {

                                    response_message.result = true;

                                    // ?????????????????? ???????????????? ????????????????????
                                    std::unordered_set<std::string> device_actions;

                                    for (auto const &action : actions) {
                                        device_actions.emplace(action.as<std::string>());
                                    }

                                    // ?????????????????? ???????????????????? ????????????????????
                                    std::unordered_set<std::string> device_variables;

                                    for (auto const &variable : variables) {
                                        device_variables.emplace(variable.as<std::string>());
                                    }

                                    pimpl->add_reg_device(key, task.transport_id_, device_actions,
                                                          device_variables);

                                } else {
                                    response_message.error = api::json_rpc::response_error(
                                            api::json_rpc::error_code::unknown_error_code,
                                            "device control contains actions and/or variables");
                                }

                            } else {
                                response_message.error = api::json_rpc::response_error(
                                        api::json_rpc::error_code::unknown_error_code,
                                        "device not found");
                            }

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

            attach(
                    behavior::make_handler("disconnect", [this](behavior::context &ctx) -> void {
                        // ?????????????????????? ???? ??????????????????

                        auto &task = ctx.message()->body<api::task>();

                        auto key = task.request.params["key"].as<std::string>();

                        try {
                            pimpl->remove_reg_device(key);
                        } catch (std::exception &e) {

                            // ???????????????????? ??????????
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
                    behavior::make_handler("request", [this](behavior::context &ctx) -> void {
                        // ???????????????? ?????????????? ???? ????????????????????

                        auto &task = ctx.message()->body<api::task>();

                        auto device_key = task.request.metadata["device-key"].as<std::string>();

                        if (pimpl->is_reg_device(device_key)) {

                            auto device_transport_id = pimpl->get_device_transport_id(device_key);

                            task.request.metadata["transport"] = task.transport_id_;

                            // ???????????????????? ???????????????? ???????????????????? - action
                            auto ws_response = new api::web_socket(device_transport_id);

                            api::json_rpc::request_message request_device_message;
                            request_device_message.id = task.request.id;
                            request_device_message.method = task.request.params["action"].as<std::string>();
                            request_device_message.metadata = api::json::json_map{
                                    {"transport", task.transport_id_}
                            };

                            ws_response->body = api::json_rpc::serialize(request_device_message);

                            api::transport ws_res(ws_response);

                            ctx->addresses("ws")->send(
                                    messaging::make_message(
                                            ctx->self(),
                                            "write",
                                            std::move(ws_res)
                                    )
                            );

                        } else {

                            auto ws_response = new api::web_socket(task.transport_id_);
                            api::json_rpc::response_message response_message;

                            response_message.id = task.request.id;
                            response_message.error = api::json_rpc::response_error(
                                    api::json_rpc::error_code::unknown_error_code,
                                    "device not connected");

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
                    behavior::make_handler("response", [this](behavior::context &ctx) -> void {
                        // ?????????????????? ???????????? ???? ????????????????????

                        auto transport = ctx.message()->body<api::transport>();

                        auto ws = std::static_pointer_cast<api::web_socket>(transport);

                        // ???????????????????? ?????????? ???? ???????????????????????? ?????????????????? ??????????????
                        api::json::json_map message{api::json::data{ws->body}};

                        if (api::json_rpc::is_response(message)) {

                            auto transports_id = message["metadata"]["transport"].as<api::transport_id>();
                            auto device_key = message["metadata"]["device-key"].as<std::string>();

                            if (pimpl->is_reg_device(device_key)) {

                                message.erase("metadata");

                                auto ws_response = new api::web_socket(transports_id);
                                ws_response->body = message.to_string();

                                api::transport ws_res(ws_response);

                                ctx->addresses("ws")->send(
                                        messaging::make_message(
                                                ctx->self(),
                                                "write",
                                                api::transport(ws_res)
                                        )
                                );
                            }
                        }
                    })
            );

            attach(
                    behavior::make_handler("notify", [this](behavior::context &ctx) -> void {
                        // ?????????????????? ?????????????????????? ???? ????????????????????

                        auto transport = ctx.message()->body<api::transport>();

                        auto ws = std::static_pointer_cast<api::web_socket>(transport);

                        api::json::json_map message{api::json::data{ws->body}};

                        std::cout << "notify: " << message << std::endl;

                        if (api::json_rpc::is_notify(message)) {

                            auto device_key = message["metadata"]["device-key"].as<std::string>();


                            if (pimpl->is_reg_device(device_key)) {

                                auto ws_notify = new api::web_socket(ws->id());
                                ws_notify->body = message.to_string();

                                api::transport ws_not(ws_notify);

                                ctx->addresses("subscriptions")->send(
                                        messaging::make_message(
                                                ctx->self(),
                                                "emit",
                                                std::move(ws_not)
                                        )
                                );
                            }
                        }
                    })
            );

            attach(
                    behavior::make_handler("subscribe", [this](behavior::context &ctx) -> void {
                        // ?????????????????? ?????????????????????? ???? ????????????????????

                        auto &task = ctx.message()->body<api::task>();

                        auto device_key = task.request.params["device-key"].as<std::string>();

                        if (pimpl->is_reg_device(device_key)) {

                            ctx->addresses("connections")->send(
                                    messaging::make_message(
                                            ctx->self(),
                                            "subscribe",
                                            std::move(task)
                                    )
                            );
                        } else {

                            auto ws_response = new api::web_socket(task.transport_id_);
                            api::json_rpc::response_message response_message;

                            response_message.id = task.request.id;
                            response_message.error = api::json_rpc::response_error(
                                    api::json_rpc::error_code::unknown_error_code,
                                    "device not connected");

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
                    behavior::make_handler("unsubscribe", [this](behavior::context &ctx) -> void {
                        // ?????????????????? ?????????????????????? ???? ????????????????????

                        auto &task = ctx.message()->body<api::task>();

                        auto device_key = task.request.params["device-key"].as<std::string>();

                        if (pimpl->is_reg_device(device_key)) {

                            ctx->addresses("connections")->send(
                                    messaging::make_message(
                                            ctx->self(),
                                            "unsubscribe",
                                            std::move(task)
                                    )
                            );
                        } else {

                            auto ws_response = new api::web_socket(task.transport_id_);
                            api::json_rpc::response_message response_message;

                            response_message.id = task.request.id;
                            response_message.error = api::json_rpc::response_error(
                                    api::json_rpc::error_code::unknown_error_code,
                                    "device not connected");

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

        void devices::startup(config::config_context_t *) {

            sqlite_config storage_config;
            storage_config.flags = OpenFlags::READWRITE | OpenFlags::CREATE | OpenFlags::FULLMUTEX;

            database db("manager_db", storage_config);

            db <<
               "create table if not exists devices ("
               "   key text primary key not null,"
               "   name text not null"
               ");";

            pimpl = std::make_unique<impl>(db);
        }

        void devices::shutdown() {

        }
    }
}