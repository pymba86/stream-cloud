
#include <messaging/blocking_mail_queue.hpp>
#include <iostream>

namespace stream_cloud {

    namespace messaging {

        enqueue_result blocking_mail_queue::put(std::shared_ptr<messaging::message> m) {
            enqueue_result status;
            {
                lock_guard lock(mutex);
                mail_queue.push_back(std::move(m));
                status = enqueue_result::success;
            }
            cv.notify_one();
            return status;
        }

        std::shared_ptr<messaging::message> blocking_mail_queue::get() {
            lock_guard lock(mutex);
            if (local_queue.empty()) {
                sync();
            }

            std::shared_ptr<message> tmp_ptr;
            if (!local_queue.empty()) {
                tmp_ptr = local_queue.front();
                local_queue.pop_front();
                return tmp_ptr;
            } else {
                return tmp_ptr;
            }
        }

        bool blocking_mail_queue::push_to_cache(std::shared_ptr<messaging::message> msg_ptr) {
            if (msg_ptr) {
                cache().push_back(std::move(msg_ptr));
                return true;
            } else {
                return false;
            }
        }

        std::shared_ptr<messaging::message> blocking_mail_queue::pop_to_cache() {
            std::shared_ptr<messaging::message> msg_ptr;
            if (!cache().empty()) {
                msg_ptr = cache().front();
                cache().pop_front();
                return msg_ptr;
            }
            return msg_ptr;
        }

        blocking_mail_queue::cache_type &blocking_mail_queue::cache() {
            return cache_;
        }


        void blocking_mail_queue::sync() {
            local_queue.splice(local_queue.begin(), mail_queue);
        }

        blocking_mail_queue::~blocking_mail_queue() = default;

        blocking_mail_queue::blocking_mail_queue() = default;


    }
}