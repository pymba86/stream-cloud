#pragma once

#include <abstract_service.hpp>

namespace stream_cloud {
    namespace router {

        class router: public config::abstract_service {
        public:
            explicit router(config::config_context_t *ctx);

            ~router();

            void startup(config::config_context_t *) override;

            void shutdown() override;
        private:
            class impl;
            std::unique_ptr<impl> pimpl;
        };
    }
}