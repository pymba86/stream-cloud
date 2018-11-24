#pragma once

#include <data_provider.hpp>

namespace stream_cloud {
    namespace providers {
        namespace http_server {

            class http_server final: public config::data_provider {
            public:
                http_server(config::config_context_t *ctx, actor::actor_address);

                ~http_server();

                void startup(config::config_context_t *) override;

                void shutdown() override;

            private:
                class impl;
                std::unique_ptr<impl> pimpl;
            };

        }
    }
}