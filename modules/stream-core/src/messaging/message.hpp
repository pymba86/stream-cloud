#pragma once

#include <string>
#include <actor/actor_address.hpp>
#include "any.hpp"
#include "message_header.hpp"

namespace stream_cloud {
    namespace messaging {

        class message final {
        public:
            message();

            message(const message &) = delete;

            message &operator=(const message &) = delete;

            message(message &&other) = default;

            message &operator=(message &&) = default;

            ~message() = default;

            message(actor::actor_address& /*sender*/, const std::string& /*name*/, any &&/*body*/);

            auto command() const noexcept -> const behavior::type_action &;

            auto sender() const -> actor::actor_address ;

            template<typename T>
            auto body() const -> const T& {
                return body_.as<T>();
            }

            template<typename T>
            auto body() -> T& {
                return body_.as<T>();
            }

            auto clone() const -> message;

            operator bool();

            void swap(message& other) noexcept;

        private:
            message(const message_header &header, const any &body);

            bool init;

            message_header header_;

            any  body_;
        };

        template <typename T>
        inline auto make_message(actor::actor_address sender_,const std::string &name, T &&data) -> message {
            return message(sender_,name, std::forward<T>(data));
        }
    }
}

inline void swap(stream_cloud::messaging::message &lhs, stream_cloud::messaging::message &rhs) noexcept {
    lhs.swap(rhs);
}