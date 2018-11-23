

#include <utility>
#include <iostream>

#include <actor/local_actor.hpp>
#include <standard_handlers/sync_contacts.hpp>
#include <standard_handlers/skip.hpp>
#include <standard_handlers/add_channel.hpp>
#include <executor/execution_device.hpp>
#include <environment/environment.hpp>
#include <messaging/message.hpp>
#include <actor/actor_address.hpp>
#include <channel/channel.hpp>

namespace stream_cloud {
    namespace actor {
        local_actor::local_actor(
                environment::abstract_environment *env,
                const std::string &type
        ) :
                abstract_actor(env, type){
            initialize();
            type_.location = locations::local;
        }

        void local_actor::initialize() {
            attach(new sync_contacts());
            attach(new skip());
            attach(new add_channel());
        }

        void local_actor::attach(behavior::abstract_action *ptr_aa) {
            reactions_.add(ptr_aa);
        }

        auto local_actor::all_view_address() const -> void  {
            for (auto &i: contacts)
                std::cout << i.first << std::endl;
        }

        local_actor::~local_actor() {

        }


        executor::execution_device *local_actor::device() const {
            return executor_;
        }

        void local_actor::device(executor::execution_device *e) {
            if (e!= nullptr) {
                executor_ = e;
            }
        }

        void local_actor::addresses(actor_address aa) {
            contacts.emplace(aa->name(), aa);
        }

        void local_actor::channel(channel::channel channel_) {
            channels.emplace(channel_->name(),channel_);
        }

        auto local_actor::addresses(const std::string &name) -> actor_address& {
            return contacts.at(name);
        }

        auto local_actor::channel(const std::string &name) -> channel::channel& {
            return channels.at(name);
        }

        std::set<std::string> local_actor::message_types() const {
            std::set<std::string> types;

            for(auto&i: reactions_) {
                types.emplace(i.first.to_string());
            }

            return types;
        }

        auto local_actor::self()  -> actor_address  {
            return address();
        }

    }
}