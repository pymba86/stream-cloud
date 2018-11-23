
#pragma once

#include <intrusive_ptr.hpp>
#include "abstract_actor.hpp"

namespace stream_cloud {
    namespace actor {

        class actor_address final {
        public:
            actor_address() = default;

            actor_address(actor_address &&) = default;

            actor_address(const actor_address &) = default;

            actor_address &operator=(actor_address &&) = default;

            actor_address &operator=(const actor_address &) = default;

            explicit actor_address(abstract_actor *aa);

            ~actor_address() = default;

            inline abstract_actor *operator->() const noexcept {
                return ptr_.get();
            }

            inline explicit operator bool() const noexcept {
                return static_cast<bool>(ptr_);
            }

            inline bool operator!() const noexcept {
                return !ptr_;
            }

        private:
            intrusive_ptr<abstract_actor> ptr_;
        };

        inline void send(actor_address& address, messaging::message&& msg){
            address->send(std::move(msg));
        }
    }
}