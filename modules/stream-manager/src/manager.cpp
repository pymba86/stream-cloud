
#include <dynamic_environment.hpp>
#include <router.hpp>
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

class storage_t final : public config::abstract_service {
public:
    explicit storage_t(config::config_context_t *ctx):  abstract_service(ctx,"storage"){
        attach(
                behavior::make_handler(
                        "dispatcher",
                        [this]( behavior::context& ctx ) -> void {

                            auto transport = ctx.message().body<api::transport>();
                            auto transport_type = transport->type();
                            std::string response = str(
                                    boost::format(R"({ "type": "manager", "data": "%1%"})") % transport->id());


                            if (transport_type == api::transport_type::http) {
                                auto http_response = new api::http(transport->id());
                                auto *http = static_cast<api::http *>(transport.get());
                                if (http->uri() == "/system") {

                                    http_response->header("Content-Type", "application/json");
                                    http_response->body(response);

                                    ctx->addresses("http")->send(
                                            messaging::make_message(
                                                    ctx->self(),
                                                    "write",
                                                    api::transport(http_response)
                                            )
                                    );

                                    api::transport_id id = 0;
                                    auto ws_response = new api::web_socket(id);
                                    ws_response->body = response;

                                    ctx->addresses("client")->send(
                                            messaging::make_message(
                                                    ctx->self(),
                                                    "write",
                                                    api::transport(ws_response)
                                            )
                                    );
                                } else {

                                    ctx->addresses("http")->send(
                                            messaging::make_message(
                                                    ctx->self(),
                                                    "close",
                                                    api::transport(http_response)
                                            )
                                    );
                                }
                            } else {
                                auto *ws = static_cast<api::web_socket *>(transport.get());
                                std::cout << ws->body << std::endl;
                            }
                        }
                )
        );


    }
    void startup(config::config_context_t *) {
    }

    void shutdown() {
    }
    ~storage_t() override = default;


private:

};




void init_service(config::dynamic_environment&env) {

    auto& storage = env.add_service<storage_t>();
    auto& ws = env.add_data_provider<client::ws_client::ws_client>(storage->entry_point());
    auto& http = env.add_data_provider<providers::http_server::http_server>(storage->entry_point());

    storage->add_shared(http.address().operator->());
    storage->add_shared(ws.address().operator->());

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