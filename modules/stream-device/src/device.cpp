
#include <dynamic_environment.hpp>

#include <http/http_server.hpp>
#include <ws/ws_server.hpp>
#include <ws/ws_client.hpp>
#include <boost/stacktrace.hpp>

#include "actor/actor.hpp"
#include <messaging/message.hpp>
#include <actor/basic_actor.hpp>

#include <transport_base.hpp>
#include <http.hpp>
#include <boost/format.hpp>
#include <transport_base.hpp>
#include "websocket.hpp"
#include <thread>

#include "manager.hpp"
#include "control.hpp"

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


void init_service(config::dynamic_environment &env) {

    // Сервисы
    auto &manager = env.add_service<device::manager>();
    auto &control = env.add_service<device::control>();


    // Поставшики данных
    auto &client_provider = env.add_data_provider<client::ws_client::ws_client>(manager->entry_point());

    // Управление
    control->add_shared(client_provider.address().operator->());

    control->join(manager);

    // Менеджер
    manager->add_shared(client_provider.address().operator->());

    manager->join(control);

}


int main(int argc, char **argv) {

    ::signal(SIGSEGV, &signal_sigsegv);

    config::configuration config;

    config::load_or_generate_config(config);

    config::dynamic_environment env(std::move(config));
    init_service(env);
    env.initialize();

    env.startup();
    return 0;


}