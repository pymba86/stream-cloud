#pragma once

#include <data_provider.hpp>


namespace stream_cloud {
    namespace client {
        namespace ws_client {

            class ws_client final : public config::data_provider {
            public:
                ws_client(config::config_context_t *, actor::actor_address);

                ~ws_client();

                void startup(config::config_context_t *ctx) override;

                void shutdown() override;

            private:
                class impl;

                std::unique_ptr<impl> pimpl;
            };
        }
    }
}