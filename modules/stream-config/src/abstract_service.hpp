#pragma once

#include <cstdint>
#include <iostream>
#include <queue>
#include <unordered_map>

#include <forward.hpp>
#include <actor/basic_actor.hpp>

namespace stream_cloud {
    namespace config {
    struct abstract_service: public actor::basic_async_actor {

            abstract_service(config_context_t *,const std::string& );

        ~abstract_service() override;

            virtual void startup(config_context_t *) = 0;

            virtual void shutdown() = 0;

            ////service_state state_;
        };
    }
}