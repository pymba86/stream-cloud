
#include <actor/async_actor.hpp>
#include <iostream>
#include <executor/execution_device.hpp>
#include <executor/abstract_coordinator.hpp>
#include <environment/environment.hpp>
#include <behavior/abstract_action.hpp>


namespace stream_cloud {
    namespace actor {
        inline void error(){
            std::cerr << " WARNING " << std::endl;
            std::cerr << " WRONG ADDRESS " << std::endl;
            std::cerr << " WARNING " << std::endl;
        }

        executor::executable_result async_actor::run(executor::execution_device *e, size_t max_throughput) {
            device(e);
            //---------------------------------------------------------------------------

            {

                messaging::message msg_ptr;
                for (size_t handled_msgs = 0; handled_msgs < max_throughput;) {
                    msg_ptr = pop_to_cache();
                    if (msg_ptr) {
                        {
                            behavior::context context_(this, std::move(msg_ptr));
                            reactions_.execute(context_); /** context processing */
                        }
                        ++handled_msgs;
                        continue;
                    }

                    msg_ptr = next_message();
                    if (msg_ptr) {
                        {
                            behavior::context context_(this, std::move(msg_ptr));
                            reactions_.execute(context_); /** context processing */
                        }
                        ++handled_msgs;

                    } else {
                        return executor::executable_result::awaiting;
                    }
                }

            }

            //---------------------------------------------------------------------------

            messaging::message msg_ptr = next_message();
            while (msg_ptr) {
                push_to_cache(std::move(msg_ptr));
                msg_ptr = next_message();
            }

            //---------------------------------------------------------------------------

            if (has_next_message()) {
                return executor::executable_result::awaiting;
            }

            return executor::executable_result::resume;
        }

        bool async_actor::send(messaging::message && mep, executor::execution_device *e) {
            mailbox().put(std::move(mep));

            if (e != nullptr) {
                device(e);
                device()->put_execute_latest(this);
            } else if(env()) {
                env()->manager_execution_device().submit(this);
            } else {
                /** local */
            }
            return true;
        }


        void async_actor::attach_to_scheduler() {
            ref();
        }

        void async_actor::detach_from_scheduler() {
            deref();
        }

        async_actor::async_actor(environment::abstract_environment *env,mailbox_type* mail_ptr, const std::string &name):
                local_actor(env, name),
                mailbox_(mail_ptr) {
        }

        void async_actor::launch(executor::execution_device *e, bool hide) {
            device(e);

            if (hide) {//TODO:???
                device()->put_execute_latest(this);
            } else {
                auto max_throughput = std::numeric_limits<size_t>::max();
                while (run(device(), max_throughput) != executor::executable_result::awaiting) {
                }
            }
        }

        bool async_actor::send(messaging::message&&msg) {
            return send(std::move(msg), nullptr);
        }

        bool async_actor::has_next_message() {
            messaging::message msg_ptr = mailbox().get();
            return push_to_cache(std::move(msg_ptr));
        }

        bool async_actor::push_to_cache(messaging::message &&msg_ptr) {
            return mailbox().push_to_cache(std::move(msg_ptr));
        }

        messaging::message async_actor::pop_to_cache() {
            return mailbox().pop_to_cache();
        }

        async_actor::mailbox_type &async_actor::mailbox() {
            return *mailbox_;
        }

        messaging::message async_actor::next_message() {
            return mailbox().get();
        }

    }
}