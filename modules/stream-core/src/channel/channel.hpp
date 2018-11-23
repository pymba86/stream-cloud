#pragma once

#include <intrusive_ptr.hpp>
#include "abstract_channel.hpp"

namespace stream_cloud {

    namespace channel {

        class channel final {
        public:

            explicit channel(abstract_channel *ac);

            abstract_channel *operator->() const noexcept;

            explicit operator bool() const noexcept;

            bool operator!() const noexcept;

        private:

            void swap(channel &other) noexcept;

            intrusive_ptr<abstract_channel> channel_;
        };

        inline void send(channel& current_channel,messaging::message&&msg){
            current_channel->send(std::move(msg));
        }

        inline void broadcast(channel& current_channel,messaging::message&&msg){
            current_channel->broadcast(std::move(msg));
        }

    }

}