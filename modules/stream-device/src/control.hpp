#pragma once

#include <abstract_service.hpp>

namespace stream_cloud {

    namespace device {

        class control: public config::abstract_service {
        public:
            explicit control(config::config_context_t *ctx);

            ~control() = default;

            void startup(config::config_context_t *) override;

            void shutdown() override;
        private:
            class impl;
            std::unique_ptr<impl> pimpl;
        };
    }
}