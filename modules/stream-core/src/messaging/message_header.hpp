#pragma once

#include <actor/actor_address.hpp>
#include <behavior/type_action.hpp>

namespace stream_cloud {

    namespace messaging {

        class message_header final {
        public:
            message_header() = default;

            message_header(const message_header &) = default;

            message_header &operator=(const message_header &) = default;

            message_header(message_header &&) = default;

            message_header &operator=(message_header &&) = default;

            ~message_header() = default;

            message_header(actor::actor_address sender_, const std::string& name);

            auto command() const noexcept -> const behavior::type_action &;

            auto sender() const -> actor::actor_address ;

        private:
            actor::actor_address sender_;
            behavior::type_action command_;
        };
    }
}