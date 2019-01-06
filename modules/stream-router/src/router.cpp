#include <utility>

#include <utility>


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
#include <sqlite_modern_cpp.h>


namespace stream_cloud {
    namespace router {

        using namespace sqlite;

        class router::impl final {
        public:
            impl(const api::json::json_map &config, sqlite::database storage)
                    : config_(config), storage_(std::move(storage)) {};

            ~impl() = default;

            api::json::json_map config_;
            sqlite::database storage_;
        };

        router::router(config::config_context_t *ctx) :
                abstract_service(ctx, "router") {

            auto config = ctx->config();

            sqlite_config storage_config;
            storage_config.flags = OpenFlags::READWRITE | OpenFlags::CREATE | OpenFlags::FULLMUTEX;
            database db("some_db", storage_config);

            db <<
               "create table if not exists user ("
               "   _id integer primary key autoincrement not null,"
               "   age int,"
               "   name text,"
               "   weight real"
               ");";


            pimpl = std::make_unique<impl>(config, db);


            attach(
                    behavior::make_handler(
                            "dispatcher",
                            [this](behavior::context &ctx) -> void {
                                auto transport = ctx.message().body<api::transport>();
                                auto transport_type = transport->type();


                                pimpl->storage_ << "insert into user (age,name,weight) values (?,?,?);"
                                   << 20
                                   << u"bob"
                                   << 83.25;


                                if (transport_type == api::transport_type::ws) {

                                    auto ws_response = new api::web_socket(transport->id());
                                    auto *ws = static_cast<api::web_socket *>(transport.get());
                                    api::task task_;
                                    api::json_rpc::parse(ws->body, task_.request);

                                    if (task_.request.method == "get_settings") {
                                        api::json_rpc::response_message response(
                                                "2",
                                                pimpl->config_);

                                        ws_response->body = api::json_rpc::serialize(response);
                                    } else {
                                        api::json_rpc::response_message response("2", "");
                                        api::json_rpc::response_error error(
                                                api::json_rpc::error_code::methodNot_found,
                                                "method not found");
                                        response.error = error;

                                        ws_response->body = api::json_rpc::serialize(response);
                                    }

                                    ctx->addresses("ws")->send(
                                            messaging::make_message(
                                                    ctx->self(),
                                                    "write",
                                                    api::transport(ws_response)
                                            )
                                    );

                                    return;
                                }


                            }
                    )
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