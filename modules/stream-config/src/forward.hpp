#pragma once

#include <environment/environment.hpp>
#include <environment/group.hpp>
#include <messaging/message.hpp>

namespace stream_cloud {
    namespace config {
        using abstract_environment = environment::abstract_environment;

        using service = environment::group;

        using message = messaging::message;

        struct config_context_t;

        struct data_provider;
    }
}