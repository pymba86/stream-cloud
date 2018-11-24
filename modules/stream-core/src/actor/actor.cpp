
#include <actor/actor.hpp>
#include "actor_address.hpp"

namespace stream_cloud {
    namespace actor {

        actor_address actor::address() const noexcept {
            return heart->address();
        };

        const std::string& actor::name() const {
            return heart->name();
        }

        void actor::swap(actor &other) noexcept {
            using std::swap;
            heart.swap(other.heart);
        }
    }
}