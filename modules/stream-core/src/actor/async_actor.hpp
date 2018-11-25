#pragma once

#include <memory>
#include <unordered_map>

#include "abstract_actor.hpp"
#include "local_actor.hpp"
#include <behavior/context.hpp>
#include <forwards.hpp>
#include <executor/executable.hpp>
#include <messaging/mail_box.hpp>
#include <executor/execution_device.hpp>

namespace stream_cloud {

    namespace actor {
        class async_actor :
                public local_actor,
                public executor::executable {
        public:

            using mailbox_type = messaging::mail_box;

            bool send(messaging::message&&) final;

            bool send(messaging::message&&, executor::execution_device *) final;

            void launch(executor::execution_device *, bool) final;

            executor::executable_result run(executor::execution_device *, size_t max_throughput) final;

            virtual ~async_actor() = default;

        protected:
            async_actor(environment::abstract_environment *, mailbox_type*, const std::string &);

        

        private:

// message processing -----------------------------------------------------

            messaging::message next_message();

            bool has_next_message();

            mailbox_type &mailbox();

            bool push_to_cache(messaging::message&&);

            messaging::message pop_to_cache();

// ----------------------------------------------------- message processing

            std::unique_ptr<mailbox_type> mailbox_;
        };
    }
}