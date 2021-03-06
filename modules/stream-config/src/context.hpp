#pragma once

#include <boost/asio.hpp>
#include <boost/thread.hpp>

#include <forwards.hpp>
#include <json/value.hpp>


namespace stream_cloud {

    namespace config {

        struct config_context_t {

            virtual auto env() -> environment::abstract_environment * = 0;

            virtual auto config() const -> api::json::json_map & = 0;

            virtual boost::asio::io_service &main_loop() const = 0;

            virtual boost::thread_group &background() const = 0;

            virtual ~config_context_t() = default;

        };
    }
}