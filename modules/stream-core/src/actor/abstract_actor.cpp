
#include <actor/abstract_actor.hpp>

namespace stream_cloud {
    namespace actor {
        abstract_actor::abstract_actor(environment::abstract_environment *env, const std::string &type):
                env_(env) {

            type_.type = abstract::actor;
            type_.name = type;
        }

        actor_address abstract_actor::address() const noexcept {
            return actor_address{const_cast<abstract_actor *>(this)};
        }

        environment::environment & abstract_actor::env() {
            return env_;
        }


        auto abstract_actor::type() const -> abstract {
            return type_.type;
        }

        auto abstract_actor::name() const -> const std::string & {
            return type_.name;
        }

        auto abstract_actor::locating() const -> locations {
            return type_.location;
        }

        auto abstract_actor::message_types() const -> std::set<std::string> {
            return std::set<std::string>();
        }

    }
}