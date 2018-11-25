#pragma once
#include <string>
#include <forwards.hpp>
#include <metadata.hpp>
#include <intrusive_ptr.hpp>

namespace stream_cloud {

    namespace channel {
        struct abstract_channel : public intrusive_base<abstract_channel> {
            virtual ~abstract_channel();

            virtual auto send(messaging::message &&)  -> bool      = 0;

            virtual auto broadcast(messaging::message &&) -> bool  = 0;

            auto type() const -> abstract ;

            auto name() const -> const std::string &;

            auto locating() const -> locations;

        protected:
            metadata type_;
        };
    }
}