#pragma once

#include <memory>
#include <ref_counted.hpp>
#include <actor/actor_address.hpp>
#include "connection_identifying.hpp"

namespace stream_cloud {
    namespace network {

        using buffer = std::string;

        struct multiplexer: public ref_counted {

            virtual ~multiplexer() = default;

            virtual std::size_t start() = 0;

            virtual void new_tcp_listener(const std::string &host, uint16_t port, const actor::actor_address &) = 0;

            virtual void new_tcp_connection(const std::string &host, uint16_t port, const actor::actor_address &) = 0;

            virtual void close(const connection_identifying &) = 0;

            virtual void write(const connection_identifying &, const buffer &) = 0;
        };

        using unique_multiplexer_ptr = std::unique_ptr<multiplexer>;

    }
}