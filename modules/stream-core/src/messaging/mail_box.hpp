#pragma once

#include "message.hpp"

namespace stream_cloud {

    namespace messaging {

        enum class enqueue_result {
            success = 0,
            unblocked_reader,
            queue_closed
        };

        struct mail_box {
            mail_box() = default;
            mail_box(const mail_box &) = delete;
            mail_box &operator=(const mail_box &)= delete;
            mail_box(mail_box &&) = delete;
            mail_box &operator=(mail_box &&)= delete;
            virtual ~ mail_box() = default;
            /// thread-safe
            virtual enqueue_result put(std::shared_ptr<messaging::message>)=0;
            virtual std::shared_ptr<messaging::message> get() = 0;
            ///non thread safe
            virtual bool push_to_cache(std::shared_ptr<messaging::message> msg_ptr) = 0;
            virtual std::shared_ptr<messaging::message> pop_to_cache() =0;
        };
    }
}