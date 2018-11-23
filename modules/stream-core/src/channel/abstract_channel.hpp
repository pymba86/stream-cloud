#pragma once
#include <string>
#include <ref_counted.hpp>
#include <forwards.hpp>
#include <metadata.hpp>

namespace stream_cloud {

    namespace channel {
        struct abstract_channel : public ref_counted {
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