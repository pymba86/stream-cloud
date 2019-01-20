#include <utility>

#include <utility>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/cxx11/any_of.hpp>

#include "manager.hpp"
#include "error.hpp"

#include <unordered_map>
#include <unordered_set>

#include <transport_base.hpp>
#include <http.hpp>
#include <boost/format.hpp>
#include <intrusive_ptr.hpp>
#include <transport_base.hpp>
#include "websocket.hpp"
#include <thread>
#include <task.hpp>
#include <json-rpc.hpp>
#include <context.hpp>
#include  <boost/beast/websocket/error.hpp>

namespace stream_cloud {
    namespace device {

        class manager::impl final {

        public:
            impl(std::string device_key, std::string manager_key, std::string profile_key)
                    : device_key(std::move(device_key)), manager_key(std::move(manager_key)),
                      profile_key(std::move(profile_key)) {};

            ~impl() = default;

            std::vector<std::string> get_service_list() const {
                return {"control"};
            }

            const std::string device_key;
            const std::string manager_key;
            const std::string profile_key;
        };

        manager::manager(config::config_context_t *ctx) :
                abstract_service(ctx, "manager") {


            attach(
                    behavior::make_handler("dispatcher", [this](behavior::context &ctx) -> void {
                        // Ответ от платформы

                        auto &transport = ctx.message().body<api::transport>();
                        auto transport_type = transport->type();

                        auto *ws = static_cast<api::web_socket *>(transport.get());

                        if (transport_type == api::transport_type::ws) {
                            api::json::json_map message{api::json::data{ws->body}};

                            std::cout << "manager: " << message << std::endl;

                            if (api::json_rpc::is_request(message)) {

                                auto id = message["metadata"]["transport"].as<api::transport_id>();



                                api::task task;
                                task.transport_id_ = id;
                                api::json_rpc::parse(message, task.request);

                                if (api::json_rpc::contains(task.request.metadata,
                                                            "transport")) {

                                    // Обрабатываем сообщение от платформы
                                    std::vector<std::string> dispatcher;
                                    boost::algorithm::split(dispatcher, task.request.method,
                                                            boost::is_any_of("."));

                                    if (dispatcher.size() > 1) {

                                        const auto service_name = dispatcher.at(0);
                                        const auto method_name = dispatcher.at(1);


                                        if (boost::algorithm::any_of_equal(pimpl->get_service_list(),
                                                                           service_name)) {

                                            auto service = ctx->addresses(service_name);

                                            if (service->message_types().count(method_name)) {

                                                service->send(
                                                        messaging::make_message(
                                                                ctx->self(),
                                                                method_name,
                                                                std::move(task)
                                                        ));


                                            } else {

                                                api::json_rpc::response_message response_message;
                                                response_message.id = task.request.id;

                                                response_message.error = api::json_rpc::response_error(
                                                        api::json_rpc::error_code::unknown_error_code,
                                                        "method not found");

                                                ws->body = api::json_rpc::serialize(response_message);

                                                auto *ws_manager = new api::web_socket(id);

                                                ctx->addresses("ws")->send(
                                                        messaging::make_message(
                                                                ctx->self(),
                                                                "write",
                                                                api::transport(ws_manager)
                                                        )
                                                );
                                            }
                                        } else {

                                            auto *ws_manager = new api::web_socket(id);

                                            api::json_rpc::response_message response_message;
                                            response_message.id = task.request.id;

                                            response_message.error = api::json_rpc::response_error(
                                                    api::json_rpc::error_code::unknown_error_code,
                                                    "service not found");

                                            ws_manager->body = api::json_rpc::serialize(response_message);

                                            ctx->addresses("ws")->send(
                                                    messaging::make_message(
                                                            ctx->self(),
                                                            "write",
                                                            api::transport(ws_manager)
                                                    )
                                            );
                                        }

                                    } else {
                                        std::cout << "not specified correctly method" << std::endl;
                                    }


                                } else if (task.request.method == "disconnect") {

                                    // Отключаемся от платформы
                                    ctx->addresses("client")->send(
                                            messaging::make_message(
                                                    ctx->self(),
                                                    "close",
                                                    std::move(std::string())
                                            )
                                    );

                                    std::cout << "device disconnect from manager" << std::endl;
                                }

                            } else if (api::json_rpc::contains(message, "error")) {
                                std::cout << "error manager: " << message << std::endl;

                                ctx->addresses("client")->send(
                                        messaging::make_message(
                                                ctx->self(),
                                                "close",
                                                std::move(std::string())
                                        )
                                );

                                std::cout << "device disconnect from manager" << std::endl;

                            } else if (api::json_rpc::is_response(message)) {
                                std::cout << "manager: " << message << std::endl;

                                std::cout << "device connecting from manager: OK" << std::endl;

                                api::json_rpc::request_message request_message;
                                request_message.id = "0";
                                request_message.method = "devices.disconnect";
                                request_message.params = api::json::json_map{{"key", pimpl->device_key}};
                                request_message.metadata = api::json::json_map{
                                        {"manager-key", pimpl->manager_key},
                                        {"user-key",    pimpl->profile_key},
                                };

                                ctx->addresses("client")->send(
                                        messaging::make_message(
                                                ctx->self(),
                                                "set_closing_message",
                                                std::move(api::json_rpc::serialize(request_message))
                                        )
                                );
                            }

                        }

                    })
            );

            attach(
                    behavior::make_handler("handshake", [this](behavior::context &ctx) -> void {
                        // Отправка команды на подключение к менеджеру

                        auto &transport = ctx.message().body<api::transport>();

                        auto ws_response = new api::web_socket(transport->id());
                        api::json_rpc::request_message request_message;
                        request_message.id = "0";
                        request_message.method = "devices.connect";
                        request_message.params = api::json::json_map{
                                {"key",       pimpl->device_key},
                                {"actions",   api::json::json_array{"control.on", "control.off"}},
                                {"variables", api::json::json_array{"control.status"}},
                        };

                        request_message.metadata = api::json::json_map{
                                {"manager-key", pimpl->manager_key},
                                {"user-key",    pimpl->profile_key},
                        };

                        ws_response->body = api::json_rpc::serialize(request_message);

                        ctx->addresses("client")->send(
                                messaging::make_message(
                                        ctx->self(),
                                        "write",
                                        api::transport(ws_response)
                                )
                        );

                        std::cout << "device connecting from manager" << std::endl;
                    })
            );

            attach(
                    behavior::make_handler("error", [this](behavior::context &ctx) -> void {
                        // Обработка ошибок

                        auto &transport = ctx.message().body<api::transport>();
                        auto transport_type = transport->type();

                        if (transport_type == api::transport_type::ws) {

                            auto *error = static_cast<api::error *>(transport.get());

                            if (error->code == boost::asio::error::connection_reset
                                || error->code == boost::asio::error::not_connected
                                || error->code == boost::asio::error::eof
                                   || error->code == boost::beast::websocket::error::closed) {
                                // Соединение сброшено на другой стороне

                                std::cout << "device disconnect from manager" << std::endl;

                            } else {

                                std::cout << "error client manager: " << error->code.message() << std::endl;

                                auto ws_response = new api::web_socket(error->id());

                                ctx->addresses("client")->send(
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


            attach(
                    behavior::make_handler("write", [this](behavior::context &ctx) -> void {
                        // Обработчик после отправки сообщения

                        auto &transport = ctx.message().body<api::transport>();
                        auto transport_type = transport->type();

                        if (transport_type == api::transport_type::ws) {

                            auto *ws = static_cast<api::web_socket *>(transport.get());

                            api::json::json_map message{api::json::data{ws->body}};

                            api::json::json_map metadata;

                            if (api::json_rpc::contains(message, "metadata")) {
                                metadata = message["metadata"].as<api::json::json_map>();
                            }

                            metadata["transport"] = ws->id();
                            metadata["device-key"] = pimpl->device_key;
                            metadata["manager-key"] = pimpl->manager_key;
                            metadata["user-key"] = pimpl->profile_key;

                            message["metadata"] = metadata;


                            auto *ws_response = new api::web_socket(ws->id());
                            ws_response->body = message.to_string();

                            ctx->addresses("client")->send(
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

        manager::~manager() {

        };

        void manager::startup(config::config_context_t *ctx) {

            auto device_key = ctx->config()["device-key"].as<std::string>();
            auto manager_key = ctx->config()["manager-key"].as<std::string>();
            auto profile_key = ctx->config()["profile-key"].as<std::string>();

            pimpl = std::make_unique<impl>(device_key, manager_key, profile_key);
        }

        void manager::shutdown() {
        }
    }
}