
#include <router.hpp>

#include <unordered_map>
#include <unordered_set>

#include <transport_base.hpp>
#include <http.hpp>

namespace stream_cloud {
    namespace router {
        class router::impl final {
        public:
            impl() = default;

            ~impl() = default;


        };

        router::router(config::config_context_t *ctx) :
                abstract_service(ctx, "router"),
                pimpl(std::make_unique<impl>()) {


            attach(
                    behavior::make_handler(
                            "dispatcher",
                            [this](behavior::context &ctx) -> void {
                                auto& transport = ctx.message().body<api::transport>();
                                auto *http = static_cast<api::http *>(transport.detach());



                                if (http->uri() == "/system") {
                                  std::cout << "Hello World";
                                    return ;
                                }
                            }
                    )
            );


        }

        router::~router() = default;

        void router::startup(config::config_context_t *) {

        }

        void router::shutdown() {

        }
    }
}