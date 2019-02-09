
#include <dynamic_environment.hpp>

#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/filesystem.hpp>

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

void terminate_handler() {
    std::cerr << "terminate called:"
              << std::endl
              << boost::stacktrace::stacktrace()
              << std::endl;
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