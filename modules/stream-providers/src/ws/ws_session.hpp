#include <utility>

#pragma once

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <transport_base.hpp>
#include <abstract_service.hpp>
#include "websocket.hpp"

namespace stream_cloud {
    namespace providers {
        namespace ws_server {

            using tcp = boost::asio::ip::tcp;               // from <boost/asio/ip/tcp.hpp>
            namespace http = boost::beast::http;            // from <boost/beast/http.hpp>
            namespace websocket = boost::beast::websocket;  // from <boost/beast/websocket.hpp>

            inline void fail(boost::system::error_code ec, char const *what) {
                std::cerr << (std::string(what) + ": " + ec.message() + "\n");
            }


            template<class NextLayer>
            inline void setup_stream(websocket::stream<NextLayer> &ws) {
                websocket::permessage_deflate pmd;
                pmd.client_enable = true;
                pmd.server_enable = true;
                pmd.compLevel = 3;
                ws.set_option(pmd);
                ws.auto_fragment(false);
                ws.read_message_max(64 * 1024 * 1024);
            }

            class ws_session : public std::enable_shared_from_this<ws_session> {

                // This queue_vm is used for HTTP pipelining.


            public:
                explicit ws_session(
                        tcp::socket,
                        api::transport_id,
                        actor::actor_address
                );

                void run();

                void write(const std::shared_ptr<api::web_socket> &ptr);

                void write(const std::string &value);

                void close();

                api::transport_id id() const;

                void on_accept(boost::system::error_code ec);

                void do_read();

                void on_read(boost::system::error_code ec, std::size_t bytes_transferred);

                void on_write(boost::system::error_code ec, std::size_t bytes_transferred);

                const api::transport_id id_;


            public:

                websocket::stream<tcp::socket> ws_;
                boost::asio::strand<boost::asio::io_context::executor_type> strand_;
                boost::beast::flat_buffer buffer_;
                actor::actor_address main_pipe_;
                std::deque<std::shared_ptr<std::string const>> _que {};
            };

        }
    }
}