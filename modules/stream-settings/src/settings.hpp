#pragma once

#include <abstract_service.hpp>

namespace stream_cloud {
    namespace settings {

        class settings: public config::abstract_service {
        public:
            explicit settings(config::config_context_t *ctx);

            ~settings();

            void startup(config::config_context_t *) override;

            void shutdown() override;
        private:
            class impl;
            std::unique_ptr<impl> pimpl;
        };
    }
}