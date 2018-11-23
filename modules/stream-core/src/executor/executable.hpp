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

            virtual void attach_to_scheduler() = 0;

            virtual void detach_from_scheduler() = 0;

            virtual executable_result run(executor::execution_device *,size_t max_throughput) = 0;
        };
    }
}