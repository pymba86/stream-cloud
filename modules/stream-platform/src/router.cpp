#include <utility>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/cxx11/any_of.hpp>

#include <router.hpp>

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
    namespace platform {

        class router::impl final {

        public:
            impl() = default;

            bool is_reg_manager(const std::string &manager_key) const {
                return manager_reg.find(manager_key) != manager_reg.end();
            }

            void add_reg_manager(const std::string &manager_key, api::transport_id transport_id) {
                manager_reg.emplace(manager_key, transport_id);
            }

            void remove_reg_manager(const std::string &manager_key) {
                manager_reg.erase(manager_key);
            }

            api::transport_id get_manager_transport_id(const std::string &key) const {
                return manager_reg.at(key);
            }

            std::vector<std::string> get_service_list() const {
                return {"profile", "manager", "settings"};
            }

            std::vector<std::string> get_method_list() const {
                return {"signup"};
            }

            ~impl() = default;

        private:
            std::unordered_map<std::string, api::transport_id> manager_reg;
        };

        router::router(config::config_context_t *ctx) :
                abstract_service(ctx, "router") {


            attach(
                    behavior::make_handler(
                            "dispatcher",
                            [this](behavior::context &ctx) -> void {

                                auto transport = ctx.message().body<api::transport>();
                                auto transport_type = transport->type();

                                if (transport_type == api::transport_type::ws) {

                                    // get transport provider
                                    auto *ws = static_cast<api::web_socket *>(transport.get());

                                    // parse request
                                    api::task task_;
                                    task_.transport_id_ = transport->id();
                                    api::json_rpc::parse(ws->body, task_.request);

                                    // get name service and method call
                                    std::vector<std::string> dispatcher;
                                    boost::algorithm::split(dispatcher, task_.request.method, boost::is_any_of("."));

                                    // check name service, method
                                    if (dispatcher.size() > 1) {

                                        if (api::json_rpc::contains(task_.request.metadata, "manager-key")) {

                                            auto manager_key = task_.request.metadata["manager-key"].get<std::string>();

                                            // Work in manager

                                            if (pimpl->is_reg_manager(manager_key)) {

                                                auto manager_transport_id = pimpl->get_manager_transport_id(
                                                        manager_key);

                                                // send request to manager
                                                auto ws_response = new api::web_socket(manager_transport_id);
                                                ws_response->body = ws->body;

                                                ctx->addresses("ws")->send(
                                                        messaging::make_message(
                                                                ctx->self(),
                                                                "write",
                                                                api::transport(ws_response)
                                                        )
                                                );

                                            } else {
                                                // Менеджер не найден или не подключен
                                            }
                                        } else {

                                            // Work in platform

                                            auto service_name = dispatcher.at(0);

                                            // check white list service name
                                            if (boost::algorithm::any_of_equal(pimpl->get_service_list(),
                                                                               service_name)) {

                                                if (api::json_rpc::contains(task_.request.metadata, "profile-key")) {

                                                    ctx->addresses("profile")->send(
                                                            messaging::make_message(
                                                                    ctx->self(),
                                                                    "authentication",
                                                                    std::move(task_)
                                                            )
                                                    );

                                                } else {
                                                    // Переход на регистрацию
                                                    auto method_name = dispatcher.at(1);

                                                    // check white list service name
                                                    if (boost::algorithm::any_of_equal(pimpl->get_method_list(),
                                                                                       method_name)) {

                                                        ctx->addresses(service_name)->send(
                                                                messaging::make_message(
                                                                        ctx->self(),
                                                                        method_name,
                                                                        std::move(task_)
                                                                )
                                                        );
                                                    } else {
                                                        auto ws_response = new api::web_socket(task_.transport_id_);
                                                        ws_response->body = "Метод не найден";
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
                                                auto ws_response = new api::web_socket(task_.transport_id_);
                                                ws_response->body = "Сервис не найден";
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
                                        // Не правильно указан метод
                                    }
                                } else if (transport_type == api::transport_type::http) {
                                    // Проверяем на trusted_url
                                    // Отдаем клиента менеджера
                                    // Отдаем клиента платформы
                                    // Отдаем статичные файлы
                                }
                            }
                    )
            );

            attach(
                    behavior::make_handler("register_manager", [this](behavior::context &ctx) -> void {
                        // Добавляем менеджер когда он подключается. Забераем ключ из metadata
                        auto &task = ctx.message().body<api::task>();
                        auto manager_key = task.request.metadata["manager-key"].as<std::string>();
                        pimpl->add_reg_manager(manager_key, task.transport_id_);

                        // Отправить статус выполнения по ws
                    })
            );

            attach(
                    behavior::make_handler("unregister_manager", [this](behavior::context &ctx) -> void {
                        // Удаляем менеджер когда он отключается Забераем ключ из metadata
                        auto &task = ctx.message().body<api::task>();
                        auto manager_key = task.request.metadata["manager-key"].as<std::string>();
                        pimpl->remove_reg_manager(manager_key);

                        // Отправить статус выполнения по ws
                    })
            );

        }

        router::~router() {

        };

        void router::startup(config::config_context_t *) {

        }

        void router::shutdown() {
        }
    }
}