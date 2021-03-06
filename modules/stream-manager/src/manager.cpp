
#include <dynamic_environment.hpp>
#include "router.hpp"
#include "users.hpp"
#include "groups.hpp"
#include "platform.hpp"
#include "subscriptions.hpp"
#include "devices.hpp"
#include "connections.hpp"
#include <http/http_server.hpp>
#include <ws/ws_server.hpp>
#include <ws/ws_client.hpp>
#include <boost/stacktrace.hpp>
#include <manager_info.hpp>

#include "actor/actor.hpp"
#include <messaging/message.hpp>
#include <actor/basic_actor.hpp>

#include <transport_base.hpp>
#include <http.hpp>
#include <boost/format.hpp>
#include <transport_base.hpp>
#include "websocket.hpp"
#include <thread>

#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/filesystem.hpp>

using namespace stream_cloud;


void signal_sigsegv(int signum) {
    boost::stacktrace::stacktrace bt;
    if (bt) {
        std::cerr << "Signal"
                  << signum
                  << " , backtrace:"
                  << std::endl
                  << boost::stacktrace::stacktrace()
                  << std::endl;
    }
    std::abort();
}

void terminate_handler() {
    std::cerr << "terminate called:"
              << std::endl
              << boost::stacktrace::stacktrace()
              << std::endl;
}


void init_service(config::dynamic_environment &env) {

    // Сервисы
    auto &router = env.add_service<manager::router>();
    auto &users = env.add_service<manager::users>();
    auto &groups = env.add_service<manager::groups>();
    auto &subscriptions = env.add_service<manager::subscriptions>();
    auto &connections = env.add_service<manager::connections>();
    auto &devices = env.add_service<manager::devices>();
    auto &platform = env.add_service<manager::platform>();
    auto &manager_info = env.add_service<system::manager_info>();

    // Поставшики данных
    auto &client_provider = env.add_data_provider<client::ws_client::ws_client>(platform->entry_point());
    auto &ws_provider = env.add_data_provider<providers::ws_server::ws_server>(
            router->entry_point(),
            std::initializer_list<actor::actor_address>{platform->entry_point()}
    );
    auto &http_provider = env.add_data_provider<providers::http_server::http_server>(router->entry_point());

    // Пользователи
    users->add_shared(ws_provider.address().operator->());
    users->add_shared(client_provider.address().operator->());

    users->join(router);
    users->join(groups);
    users->join(subscriptions);
    users->join(connections);
    users->join(devices);

    // Группы
    groups->add_shared(ws_provider.address().operator->());
    groups->add_shared(client_provider.address().operator->());

    groups->join(router);
    groups->join(users);
    groups->join(subscriptions);
    groups->join(connections);
    groups->join(devices);

    // Подписки
    subscriptions->add_shared(ws_provider.address().operator->());
    subscriptions->add_shared(client_provider.address().operator->());

    subscriptions->join(router);
    subscriptions->join(users);
    subscriptions->join(groups);
    subscriptions->join(connections);
    subscriptions->join(devices);

    // Подключения
    connections->add_shared(ws_provider.address().operator->());
    connections->add_shared(client_provider.address().operator->());

    connections->join(router);
    connections->join(users);
    connections->join(groups);
    connections->join(subscriptions);
    connections->join(devices);

    // Устройства
    devices->add_shared(ws_provider.address().operator->());
    devices->add_shared(client_provider.address().operator->());

    devices->join(router);
    devices->join(users);
    devices->join(groups);
    devices->join(subscriptions);
    devices->join(connections);

    // Платформа
    platform->add_shared(ws_provider.address().operator->());
    platform->add_shared(client_provider.address().operator->());

    platform->join(router);
    platform->join(users);
    platform->join(groups);
    platform->join(subscriptions);
    platform->join(connections);

    // Роутер
    router->add_shared(http_provider.address().operator->());
    router->add_shared(ws_provider.address().operator->());
    router->add_shared(client_provider.address().operator->());

    router->join(users);
    router->join(groups);
    router->join(subscriptions);
    router->join(connections);
    router->join(devices);
    router->join(manager_info);

    // Менеджер инфо
    manager_info->add_shared(ws_provider.address().operator->());
    manager_info->join(router);

}


int main(int argc, char **argv) {

    ::signal(SIGSEGV, &signal_sigsegv);
    std::set_terminate(terminate_handler);

    boost::program_options::variables_map args_;
    boost::program_options::options_description app_options_;

    app_options_.add_options()
            ("help", "Print help messages")
            ("data-dir", "data-dir");

    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, app_options_), args_);

    if (args_.count("help")) {
        std::cout << "command line parameter" << std::endl << app_options_ << std::endl;
        return 0;
    }

    config::configuration config;

    if (args_.count("data-dir")) {
        auto dir  = args_["data-dir"].as<std::string>();
        config::load_or_generate_config(config, boost::filesystem::path(dir));

    } else {
        config::load_or_generate_config(config, boost::filesystem::current_path());
    }

    config::dynamic_environment env(std::move(config));
    init_service(env);
    env.initialize();

    env.startup();
    return 0;

}