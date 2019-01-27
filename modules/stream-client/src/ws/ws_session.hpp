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
    namespace client {
        namespace ws_client {

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
                ws.auto_fragment(true);
                ws.read_message_max(64 * 1024 * 1024);
            }

            class ws_session : public std::enable_shared_from_this<ws_session> {


            public:
                explicit ws_session(boost::asio::io_context &ioc,
                                    api::transport_id id,
                                    actor::actor_address pipe_
                );

                void run(const std::string &host_, const std::string &text_);

                void write(const std::shared_ptr<api::web_socket> &ptr);

                void write(std::string const&& str_);

                void do_read();

                void on_read(boost::system::error_code ec, std::size_t bytes_transferred);

                void on_write(boost::system::error_code ec, std::size_t bytes_transferred);

                void on_resolve(boost::system::error_code ec, tcp::resolver::results_type results);

                void on_connect(boost::system::error_code ec);

                void on_handshake(boost::system::error_code ec);

                void close();

            public:

                const api::transport_id id_;


            public:

                websocket::stream<tcp::socket> ws_;
                tcp::resolver resolver_;
                boost::beast::multi_buffer buffer_;
                boost::asio::strand<boost::asio::io_context::executor_type> strand_;
                actor::actor_address pipe_;
                std::string host_;
                std::string port_;
                std::mutex mutex_;

                std::deque<std::shared_ptr<std::string const>> _que {};
            };


        }
    }
}