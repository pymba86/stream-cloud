#pragma once

#include <mutex>
#include <condition_variable>
#include <list>
#include <memory>
#include <atomic>
#include "mail_box.hpp"


namespace stream_cloud {

    namespace messaging {

        class blocking_mail_queue final : public mail_box {
        public:
            using cache_type = std::list<std::shared_ptr<messaging::message>>;

            using queue_base_type = std::list<std::shared_ptr<messaging::message>>;

            using unique_lock = std::unique_lock<std::mutex>;
            using lock_guard = std::lock_guard<std::mutex>;
            blocking_mail_queue();
            ~blocking_mail_queue();

            enqueue_result put(std::shared_ptr<messaging::message> m);

            std::shared_ptr<messaging::message> get();

            bool push_to_cache(std::shared_ptr<messaging::message> msg_ptr);

            std::shared_ptr<messaging::message> pop_to_cache();

        private:

            cache_type &cache();

            void sync();

            mutable std::mutex mutex;
            std::condition_variable cv;

            queue_base_type mail_queue;
            queue_base_type local_queue;
            cache_type cache_;
        };
    }

}