#include <utility>

#include <utility>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/cxx11/any_of.hpp>
#include  <boost/beast/websocket/error.hpp>
#include "router.hpp"
#include <error.hpp>
#include <unordered_map>
#include <unordered_set>

#include <transport_base.hpp>
#include <http.hpp>
#include <boost/format.hpp>
#include <transport_base.hpp>
#include "websocket.hpp"
#include <thread>
#include <task.hpp>
#include <json-rpc.hpp>
#include <context.hpp>

namespace stream_cloud {
    namespace manager {

        class router::impl final {

        public:
            impl(std::string profile_key, std::string manager_key)
                    : profile_key(std::move(profile_key)), manager_key(std::move(manager_key)) {};

            std::vector<std::string> get_service_list() const {
                return {"users", "devices", "groups", "subscriptions", "connections", "devices"};
            }

            std::vector<std::string> get_quest_method_list() const {
                return {"users.login", "manager.info"};
            }

            std::vector<std::string> get_user_method_list() const {
                return {"devices.list", "devices.control", "devices.detail", "devices.subscribe",
                        "subscriptions.devices", "devices.unsubscribe", "connections.subscribe",
                        "connections.unsubscribe"};
            }

            ~impl() = default;

            const std::string profile_key;
            const std::string manager_key;

        };

        router::router(config::config_context_t *ctx) :
                abstract_service(ctx, "router") {


            attach(
                    behavior::make_handler(
                            "dispatcher",
                            [this](behavior::context &ctx) -> void {

                                auto transport = ctx.message()->body<api::transport>();
                                auto transport_type = transport->type();

                                if (transport_type == api::transport_type::ws) {

                                    auto ws = std::static_pointer_cast<api::web_socket>(transport);

                                    api::json::json_map message{api::json::data{ws->body}};

                                    if (api::json_rpc::is_request(message)) {

                                        api::task task_;
                                        task_.transport_id_ = ws->id();
                                        api::json_rpc::parse(message, task_.request);


                                        std::vector<std::string> dispatcher;
                                        boost::algorithm::split(dispatcher, task_.request.method,
                                                                boost::is_any_of("."));

                                        if (dispatcher.size() > 1) {

                                            const auto service_name = dispatcher.at(0);
                                            const auto method_name = dispatcher.at(1);

                                            task_.storage.emplace("service.name", service_name);
                                            task_.storage.emplace("service.method", method_name);
                                            task_.storage.emplace("profile.key", pimpl->profile_key);

                                            if (api::json_rpc::contains(task_.request.metadata,
                                                                        "user-key")) {

                                                // ???????????????? ????????????????
                                                ctx->addresses("users")->send(
                                                        messaging::make_message(
                                                                ctx->self(),
                                                                "auth",
                                                                std::move(task_)
                                                        )
                                                );

                                            } else {

                                                if (boost::algorithm::any_of_equal(
                                                        pimpl->get_quest_method_list(),
                                                        task_.request.method)) {

                                                    ctx->addresses(service_name)->send(
                                                            messaging::make_message(
                                                                    ctx->self(),
                                                                    method_name,
                                                                    std::move(task_)
                                                            )
                                                    );
                                                } else {

                                                    // ???????????? ????????????????
                                                    auto ws_response = new api::web_socket(task_.transport_id_);
                                                    api::json_rpc::response_message response_message;
                                                    response_message.id = task_.request.id;

                                                    response_message.error = api::json_rpc::response_error(
                                                            api::json_rpc::error_code::unknown_error_code,
                                                            "access is denied");

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

                                        } else {

                                            // ???? ?????????????????? ???????????? ??????????
                                            auto ws_response = new api::web_socket(task_.transport_id_);
                                            api::json_rpc::response_message response_message;
                                            response_message.id = task_.request.id;

                                            response_message.error = api::json_rpc::response_error(
                                                    api::json_rpc::error_code::unknown_error_code,
                                                    "not specified correctly method");

                                            ws_response->body = api::json_rpc::serialize(response_message);

                                            ctx->addresses("ws")->send(
                                                    messaging::make_message(
                                                            ctx->self(),
                                                            "write",
                                                            api::transport(ws_response)
                                                    )
                                            );
                                        }

                                    } else if (api::json_rpc::is_response(message)) {
                                        // ?????????? ???? ????????????????????

                                        auto ws_response = new api::web_socket(ws->id());
                                        ws_response->body = message.to_string();

                                        ctx->addresses("devices")->send(
                                                messaging::make_message(
                                                        ctx->self(),
                                                        "response",
                                                        api::transport(ws_response)
                                                )
                                        );
                                    } else if (api::json_rpc::is_notify(message)) {
                                        // ?????????????????????? ???? ????????????????????

                                        auto ws_notify = new api::web_socket(ws->id());
                                        ws_notify->body = message.to_string();

                                        ctx->addresses("devices")->send(
                                                messaging::make_message(
                                                        ctx->self(),
                                                        "notify",
                                                        api::transport(ws_notify)
                                                )
                                        );
                                    }
                                } else if (transport_type == api::transport_type::http) {
                                    // ?????????????????? ???? trusted_url
                                    // ???????????? ?????????????? ??????????????????
                                    // ???????????? ?????????????? ??????????????????
                                    // ???????????? ?????????????????? ??????????
                                } else {
                                    // ???????????????????????????? ????????????
                                }
                            }
                    )
            );

            attach(
                    behavior::make_handler("service", [this](behavior::context &ctx) -> void {

                        auto &task = ctx.message()->body<api::task>();

                        auto service_name = task.storage["service.name"];
                        auto method_name = task.storage["service.method"];


                        if (boost::algorithm::any_of_equal(pimpl->get_service_list(),
                                                           service_name)) {

                            auto service = ctx->addresses(service_name);

                            if (service->message_types().count(method_name)) {


                                if (task.storage["user.admin"] == "1") {
                                    service->send(
                                            messaging::make_message(
                                                    ctx->self(),
                                                    method_name,
                                                    std::move(task)
                                            ));
                                } else if (boost::algorithm::any_of_equal(
                                        pimpl->get_user_method_list(),
                                        task.request.method)) {

                                    ctx->addresses(service_name)->send(
                                            messaging::make_message(
                                                    ctx->self(),
                                                    method_name,
                                                    std::move(task)
                                            )
                                    );

                                } else {
                                    auto ws_response = new api::web_socket(task.transport_id_);
                                    api::json_rpc::response_message response_message;
                                    response_message.id = task.request.id;

                                    response_message.error = api::json_rpc::response_error(
                                            api::json_rpc::error_code::unknown_error_code,
                                            "permission denied");

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
                                        "method not found");

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
                                    "service not found");

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
                    behavior::make_handler("error", [this](behavior::context &ctx) -> void {
                        // ?????????????????? ????????????

                        auto &transport = ctx.message()->body<api::transport>();
                        auto transport_type = transport->type();

                        if (transport_type == api::transport_type::ws) {

                            auto *error = static_cast<api::error *>(transport.get());

                            if (error->code == boost::asio::error::connection_reset
                                || error->code == boost::asio::error::not_connected
                                || error->code == boost::beast::websocket::error::closed) {
                                // ???????????????????? ???????????????? ???? ???????????? ??????????????

                                auto ws_response = new api::web_socket(error->id());

                                ctx->addresses("ws")->send(
                                        messaging::make_message(
                                                ctx->self(),
                                                "remove",
                                                api::transport(ws_response)
                                        )
                                );

                            } else {
                                auto ws_response = new api::web_socket(error->id());

                                ctx->addresses("ws")->send(
                                        messaging::make_message(
                                                ctx->self(),
                                                "close",
                                                api::transport(ws_response)
                                        )
                                );
                            }
                        }
                    })
            );
        }

        router::~router() {

        };

        void router::startup(config::config_context_t *ctx) {

            auto profile_key = ctx->config()["profile-key"].as<std::string>();
            auto manager_key = ctx->config()["manager-key"].as<std::string>();

            pimpl = std::make_unique<impl>(profile_key, manager_key);
        }

        void router::shutdown() {
        }
    }
}