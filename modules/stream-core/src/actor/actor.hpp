#pragma once

#include <string>
#include <intrusive_ptr.hpp>
#include "forwards.hpp"

namespace stream_cloud {
    namespace actor {
        class actor final {
        public:
            actor() = default;

            actor(const actor &a) = delete;

            actor(actor &&a) = default;

            actor &operator=(const actor &a) = delete;

            actor &operator=(actor &&a) = default;

            template<class T>
            explicit  actor(intrusive_ptr <T> ptr) : heart(std::move(ptr)) {}

            template<class T>
            explicit actor(T *ptr) : heart(ptr) {}

            template<class T>
            actor &operator=(intrusive_ptr <T> ptr) {
                actor tmp{std::move(ptr)};
                swap(tmp);
                return *this;
            }

            template<class T>
            actor &operator=(T *ptr) {
                actor tmp{ptr};
                swap(tmp);
                return *this;
            }

            actor_address address() const noexcept;

            ~actor() = default;

            inline abstract_actor *operator->() const noexcept {
                return heart.get();
            }

            inline explicit operator bool() const noexcept {
                return static_cast<bool>(heart);
            }

            const std::string &name() const;

            inline bool operator!() const noexcept {
                return !heart;
            }

        private:

            void swap(actor &) noexcept;

            intrusive_ptr <abstract_actor> heart;
        };
    }
}