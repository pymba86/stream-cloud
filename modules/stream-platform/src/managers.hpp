#pragma once

#include <abstract_service.hpp>

namespace stream_cloud {

    namespace platform {

        class managers: public config::abstract_service {
        public:
            explicit managers(config::config_context_t *ctx);

            ~managers() = default;

            void startup(config::config_context_t *) override;

            void shutdown() override;
        private:
            class impl;
            std::unique_ptr<impl> pimpl;
        };
    }
}