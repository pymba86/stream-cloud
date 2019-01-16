#pragma once

#include <abstract_service.hpp>

namespace stream_cloud {

    namespace manager {

        class subscriptions: public config::abstract_service {
        public:
            explicit subscriptions(config::config_context_t *ctx);

            ~subscriptions() = default;

            void startup(config::config_context_t *) override;

            void shutdown() override;
        private:
            class impl;
            std::unique_ptr<impl> pimpl;
        };
    }
}