#pragma once

#include <forwards.hpp>
#include <cstdint>
#include <cstddef>

namespace stream_cloud {

    namespace executor {
        enum class executable_result : uint8_t {
            resume,
            awaiting,
            done,
            shutdown
        };


        struct executable {
            virtual ~executable() = default;

            virtual executable_result run(executor::execution_device *,size_t max_throughput) = 0;
        };
    }
}