#pragma once

#include <string>

namespace stream_cloud {

    using actor_id = std::size_t;

    enum class abstract : uint8_t {
        non = 0x00,
        actor,
        channel
    };

    enum class locations : uint8_t {
        non = 0x00,
        local,
        remote,
    };

    struct metadata final {
        abstract type;
        locations location;
        std::string name;
    };

}