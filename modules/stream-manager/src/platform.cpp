#include <utility>

#include <utility>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/cxx11/any_of.hpp>

#include "platform.hpp"

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
    namespace manager {

        class platform::impl final {

        public:
            impl(std::string profile_key, std::string manager_key)
                    : profile_key(std::move(profile_key)), manager_key(std::move(manager_key)) {};

            ~impl() = default;

            const std::string profile_key;
            const std::string manager_key;
        };

        platform::platform(config::config_context_t *ctx) :
                abstract_service(ctx, "platform") {


            attach(
                    behavior::make_handler("dispatcher", [this](behavior::context &ctx) -> void {
                        // Ответ от платформы

                        auto transport = ctx.message().body<api::transport>();
                        auto transport_type = transport->type();

                        auto *ws = static_cast<api::web_socket *>(transport.get());

                        if (transport_type == api::transport_type::ws) {
                            api::json::json_map message{api::json::data{ws->body}};

                            if (api::json_rpc::is_request(message)) {

                                // Если есть manager-key - отпраявляем router:dispatcher

                                api::task task_;
                                task_.transport_id_ = ws->id();
                                api::json_rpc::parse(message, task_.request);

                                // Отключаемся от платформы
                                if (task_.request.method == "disconnect") {

                                    ctx->addresses("client")->send(
                                            messaging::make_message(
                                                    ctx->self(),
                                                    "close",
                                                    std::move(std::string())
                                            )
                                    );

                                    std::cout << "manager disconnect from platform" << std::endl;
                                }

                            } else if (api::json_rpc::contains(message, "error")) {
                                std::cout << "error platform: " << message << std::endl;

                                ctx->addresses("client")->send(
                                        messaging::make_message(
                                                ctx->self(),
                                                "close",
                                                std::move(std::string())
                                        )
                                );

                                std::cout << "manager disconnect from platform" << std::endl;

                            } else {
                                std::cout << "platform: " << message << std::endl;


                                // FIXME При каждом ответе устанавливается значение. Убрать
                                api::json_rpc::request_message request_message;
                                request_message.id = "0";
                                request_message.method = "managers.disconnect";
                                request_message.params = api::json::json_map{{"key", pimpl->manager_key}};
                                request_message.metadata = api::json::json_map{{"profile-key", pimpl->profile_key}};

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
                        // Отправка команды на подключение к платформе

                        auto &transport = ctx.message().body<api::transport>();

                        auto ws_response = new api::web_socket(transport->id());
                        api::json_rpc::request_message request_message;
                        request_message.id = "0";
                        request_message.method = "managers.connect";
                        request_message.params = api::json::json_map{{"key", pimpl->manager_key}};
                        request_message.metadata = api::json::json_map{{"profile-key", pimpl->profile_key}};

                        ws_response->body = api::json_rpc::serialize(request_message);

                        ctx->addresses("client")->send(
                                messaging::make_message(
                                        ctx->self(),
                                        "write",
                                        api::transport(ws_response)
                                )
                        );
                    })
            );

        }

        platform::~platform() {

        };

        void platform::startup(config::config_context_t *ctx) {

            auto profile_key = ctx->config()["profile-key"].as<std::string>();
            auto manager_key = ctx->config()["manager-key"].as<std::string>();

            pimpl = std::make_unique<impl>(profile_key, manager_key);
        }

        void platform::shutdown() {
        }
    }
}