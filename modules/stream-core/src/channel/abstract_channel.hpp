#pragma once
#include <string>
#include <forwards.hpp>
#include <metadata.hpp>
#include <memory>

namespace stream_cloud {

    namespace channel {
    struct abstract_channel : public std::enable_shared_from_this<abstract_channel> {
            virtual ~abstract_channel();

            virtual auto send(std::shared_ptr<messaging::message>)  -> bool      = 0;

            virtual auto broadcast(std::shared_ptr<messaging::message>) -> bool  = 0;

            auto type() const -> abstract ;

            auto name() const -> const std::string &;

            auto locating() const -> locations;

        protected:
            metadata type_;
        };
    }
}