#pragma once

#include <abstract_service.hpp>

namespace stream_cloud {

    namespace manager {

        class users: public config::abstract_service {
        public:
            explicit users(config::config_context_t *ctx);

            ~users() = default;

            void startup(config::config_context_t *) override;

            void shutdown() override;
        private:
            class impl;
            std::unique_ptr<impl> pimpl;
        };
    }
}