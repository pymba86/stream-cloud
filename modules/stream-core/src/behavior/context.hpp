#pragma once

#include <string>
#include <actor/actor_address.hpp>
#include <stack>
#include <memory>
#include <messaging/message.hpp>

namespace stream_cloud {

    namespace behavior {

        struct context_t {

            virtual ~context_t() = default;

            virtual void addresses(actor::actor_address) = 0;

            virtual auto addresses(const std::string &) -> actor::actor_address & = 0;

            virtual auto channel(channel::channel) -> void = 0;

            virtual auto channel(const std::string &) -> channel::channel & = 0;

            virtual auto self() -> actor::actor_address = 0;

       };


        class context final {
        public:

            context() = default;

            context(context_t* ptr,std::shared_ptr<messaging::message>);

            ~context();

            std::shared_ptr<messaging::message> message();

            const std::shared_ptr<messaging::message> message() const;

            auto operator ->() noexcept -> context_t*;

            auto operator ->() const noexcept -> context_t*;

            auto operator *() noexcept -> context_t&;

            auto operator *() const noexcept -> context_t&;

        private:
            std::unique_ptr<context_t> ptr;
            std::shared_ptr<messaging::message> msg;
        };

    }
}