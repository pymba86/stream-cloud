#pragma once

#include <abstract_service.hpp>

namespace stream_cloud {

    namespace system {

        class manager_info: public config::abstract_service {
        public:
            explicit manager_info(config::config_context_t *ctx);

            ~manager_info() = default;

            void startup(config::config_context_t *) override;

            void shutdown() override;
        private:
            class impl;
            std::unique_ptr<impl> pimpl;
        };
    }
}