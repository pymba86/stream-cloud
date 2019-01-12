#pragma once

#include <abstract_service.hpp>

namespace stream_cloud {

    namespace platform {

        class profile: public config::abstract_service {
        public:
            explicit profile(config::config_context_t *ctx);

            ~profile() = default;

            void startup(config::config_context_t *) override;

            void shutdown() override;
        private:
            class impl;
            std::unique_ptr<impl> pimpl;
        };
    }
}