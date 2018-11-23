#pragma once

#include <memory>
#include <unordered_map>
#include "abstract_actor.hpp"
#include <behavior/context.hpp>
#include <behavior/reactions.hpp>
#include <executor/execution_device.hpp>
#include <actor/actor_address.hpp>
namespace stream_cloud {

    namespace actor {
        class local_actor :
                public abstract_actor ,
                public behavior::context_t {
        public:
            //hide
            virtual void launch(executor::execution_device*, bool) = 0;

            /*
            * debug method
            */
            auto all_view_address() const -> void;

            auto message_types() const -> std::set<std::string> final;

            ~local_actor() override = default;

        protected:

            void addresses(actor_address) final;

            void channel(channel::channel) final;

            auto addresses(const std::string&)-> actor_address& final;

            auto channel(const std::string&)->channel::channel& final;

            auto self()  -> actor_address;

            void device(executor::execution_device* e);

            executor::execution_device* device() const;

            void attach(behavior::abstract_action *);

            local_actor(environment::abstract_environment *,  const std::string &);

            virtual void initialize();


        protected:
            behavior::reactions reactions_;
            std::unordered_map<std::string, actor_address> contacts;
            std::unordered_map<std::string, channel::channel> channels;
        private:
            executor::execution_device *executor_;
        };
    }
}