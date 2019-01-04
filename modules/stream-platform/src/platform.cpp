
#include <dynamic_environment.hpp>
#include <router.hpp>
#include <http/http_server.hpp>
#include <ws/ws_server.hpp>

#include <map>

#include <boost/stacktrace.hpp>


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

    auto &router = env.add_service<router::router>();
    auto &http = env.add_data_provider<providers::http_server::http_server>(router->entry_point());
    auto &ws = env.add_data_provider<providers::ws_server::ws_server>(router->entry_point());

    router->add_shared(http.address().operator->());
    router->add_shared(ws.address().operator->());

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

