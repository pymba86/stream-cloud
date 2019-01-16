#pragma once

#include <abstract_service.hpp>

namespace stream_cloud {
    namespace manager {

        class platform: public config::abstract_service {
        public:
            explicit platform(config::config_context_t *ctx);

            ~platform();

            void startup(config::config_context_t *) override;

            void shutdown() override;
        private:
            class impl;
            std::unique_ptr<impl> pimpl;
        };
    }
}