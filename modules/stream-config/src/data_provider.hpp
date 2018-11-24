#pragma once

#include <cstdint>
#include <iostream>
#include <queue>
#include <unordered_map>

#include <actor/local_actor.hpp>
#include <messaging/message.hpp>
#include <context.hpp>

namespace stream_cloud {
    namespace config {
        using messaging::message;

        using sync_actor = actor::local_actor;

        struct data_provider : public sync_actor {

            data_provider(config_context_t *context, const std::string &name);

            virtual ~data_provider();

            virtual void startup(config_context_t *) = 0;

            virtual void shutdown() = 0;

            bool send(messaging::message &&, executor::execution_device *) override;

            void launch(executor::execution_device *, bool) override;

            bool send(message &&) override;

        };
    }
}