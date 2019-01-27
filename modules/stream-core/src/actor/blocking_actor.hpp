#pragma once

#include <memory>
#include <unordered_map>
#include "abstract_actor.hpp"
#include "local_actor.hpp"
#include <behavior/context.hpp>
#include <behavior/reactions.hpp>
#include <executor/executable.hpp>
#include <messaging/mail_box.hpp>

namespace stream_cloud {

    namespace actor {
        class blocking_actor  :
                public local_actor ,
                executor::executable {
        public:

            using mailbox_type = messaging::mail_box;

            blocking_actor(environment::abstract_environment *,mailbox_type* , const std::string &);
            executor::executable_result run(executor::execution_device *, size_t) override final;
            void launch(executor::execution_device *,bool) override final ;
            virtual ~blocking_actor();

            std::shared_ptr<messaging::message> next_message();

            mailbox_type &mailbox();

        private:
            std::unique_ptr<mailbox_type> mailbox_;
        };
    }
}