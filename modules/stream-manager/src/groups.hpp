#pragma once

#include <abstract_service.hpp>

namespace stream_cloud {

    namespace manager {

        class groups: public config::abstract_service {
        public:
            explicit groups(config::config_context_t *ctx);

            ~groups() = default;

            void startup(config::config_context_t *) override;

            void shutdown() override;
        private:
            class impl;
            std::unique_ptr<impl> pimpl;
        };
    }
}