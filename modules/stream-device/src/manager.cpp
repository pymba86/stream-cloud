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

namespace stream_cloud {
    namespace device {

        class manager::impl final {

        public:
            impl(std::string device_key, std::string manager_key, std::string profile_key)
                    : device_key(std::move(device_key)), manager_key(std::move(manager_key)),
                      profile_key(std::move(profile_key)) {};

            ~impl() = default;

            const std::string device_key;
            const std::string manager_key;
            const std::string profile_key;
        };

        manager::manager(config::config_context_t *ctx) :
                abstract_service(ctx, "manager") {


            attach(
                    behavior::make_handler("dispatcher", [this](behavior::context &ctx) -> void {
                        // Ответ от платформы

                        auto transport = ctx.message().body<api::transport>();
                        auto transport_type = transport->type();

                        auto *ws = static_cast<api::web_socket *>(transport.get());

                        if (transport_type == api::transport_type::ws) {
                            api::json::json_map message{api::json::data{ws->body}};

                            if (api::json_rpc::is_request(message)) {


                                api::task task_;
                                task_.transport_id_ = ws->id();
                                api::json_rpc::parse(message, task_.request);

                                if (api::json_rpc::contains(task_.request.metadata,
                                                            "transport")) {
                                    // Обрабатываем сообщение от платформы

                                    auto id = message["metadata"]["transport"].as<api::transport_id>();
                                    auto *ws_platform = new api::web_socket(id);
                                    ws_platform->body = message.to_string();

                                    ctx->addresses("control")->send(
                                            messaging::make_message(
                                                    ctx->self(),
                                                    "название метода",
                                                    api::transport(ws_platform)
                                            )
                                    );

                                } else if (task_.request.method == "disconnect") {

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
                                request_message.params = api::json::json_map{{"key", pimpl->manager_key}};
                                request_message.metadata = api::json::json_map{
                                    {"manager-key", pimpl->manager_key},
                                    {"user-key", pimpl->profile_key},
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
                                {"actions",   api::json::json_array{"on", "off"}},
                                {"variables", api::json::json_array{"status"}},
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

                        auto transport = ctx.message().body<api::transport>();
                        auto transport_type = transport->type();

                        if (transport_type == api::transport_type::ws) {

                            auto *error = static_cast<api::error *>(transport.get());

                            if (error->code == boost::asio::error::connection_reset
                                || error->code == boost::asio::error::not_connected
                                || error->code == boost::asio::error::eof) {
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