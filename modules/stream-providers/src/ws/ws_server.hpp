#pragma once

#include <data_provider.hpp>

namespace stream_cloud {
    namespace providers {
        namespace ws_server {
        class ws_server final : public config::data_provider {
            public:
                ws_server(config::config_context_t *, actor::actor_address address, std::initializer_list<actor::actor_address>);

                ~ws_server();

                void startup(config::config_context_t *ctx) override;

                void shutdown() override;

            private:
                class impl;

                std::unique_ptr<impl> pimpl;
            };
        }
    }
}