#pragma once

#include <abstract_service.hpp>

namespace stream_cloud {
    namespace device {

        class manager: public config::abstract_service {
        public:
            explicit manager(config::config_context_t *ctx);

            ~manager();

            void startup(config::config_context_t *) override;

            void shutdown() override;
        private:
            class impl;
            std::unique_ptr<impl> pimpl;
        };
    }
}