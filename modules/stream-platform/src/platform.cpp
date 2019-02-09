
#include <dynamic_environment.hpp>
#include <router.hpp>
#include <http/http_server.hpp>
#include <ws/ws_server.hpp>
#include <manager_info.hpp>
#include "profile.hpp"
#include "managers.hpp"

#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/filesystem.hpp>

#include <map>

#include <boost/stacktrace.hpp>
#include "router.hpp"


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

    auto &router = env.add_service<platform::router>();
    auto &http = env.add_data_provider<providers::http_server::http_server>(router->entry_point());
    auto &ws = env.add_data_provider<providers::ws_server::ws_server>(router->entry_point(), std::initializer_list<actor::actor_address>{});
    auto &profile = env.add_service<platform::profile>();
    auto &managers = env.add_service<platform::managers>();
    auto &manager_info = env.add_service<system::manager_info>();

    // Профиль
    profile->add_shared(ws.address().operator->());
    profile->join(router);

    // Менеджер
    managers->add_shared(ws.address().operator->());
    managers->add_shared(http.address().operator->());
    managers->join(router);

    // Роутер
    router->add_shared(http.address().operator->());
    router->add_shared(ws.address().operator->());

    router->join(profile);
    router->join(managers);
    router->join(manager_info);


    // Менеджер инфо
    manager_info->add_shared(ws.address().operator->());
    manager_info->join(router);

}


int main(int argc, char **argv) {
    ::signal(SIGSEGV, &signal_sigsegv);
  //  std::set_terminate(terminate_handler);

    boost::program_options::variables_map args_;
    boost::program_options::options_description app_options_;

    app_options_.add_options()
            ("help,h", "Print help messages")
            ("data-dir,d", boost::program_options::value<std::string>(), "data-dir");

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

