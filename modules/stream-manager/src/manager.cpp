
#include <dynamic_environment.hpp>
#include "router.hpp"
#include "users.hpp"
#include "groups.hpp"
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
#include <intrusive_ptr.hpp>
#include <transport_base.hpp>
#include "websocket.hpp"
#include <thread>

using namespace stream_cloud;


void signal_sigsegv(int signum){
    boost::stacktrace::stacktrace bt ;
    if(bt){
        std::cerr << "Signal"
                  << signum
                  << " , backtrace:"
                  << std::endl
                  << boost::stacktrace::stacktrace()
                  << std::endl;
    }
    std::abort();
}



void init_service(config::dynamic_environment&env) {

    auto &router = env.add_service<manager::router>();
    auto& client = env.add_data_provider<client::ws_client::ws_client>(router->entry_point());
    auto &ws = env.add_data_provider<providers::ws_server::ws_server>(router->entry_point());
    auto& http = env.add_data_provider<providers::http_server::http_server>(router->entry_point());
    auto &users = env.add_service<manager::users>();
    auto &groups = env.add_service<manager::groups>();

    // Пользователи
    users->add_shared(ws.address().operator->());
    users->add_shared(client.address().operator->());
    users->join(router);
    users->join(groups);

    // Группы
    groups->add_shared(ws.address().operator->());
    groups->add_shared(client.address().operator->());
    groups->join(router);
    groups->join(users);

    // Роутер
    router->add_shared(http.address().operator->());
    router->add_shared(ws.address().operator->());
    router->add_shared(client.address().operator->());
    router->join(users);

}


int main(int argc, char **argv) {

    ::signal(SIGSEGV,&signal_sigsegv);

    config::configuration config;

    config::load_or_generate_config(config);

    config::dynamic_environment env(std::move(config));
    init_service(env);
    env.initialize();

    env.startup();
    return 0;


}