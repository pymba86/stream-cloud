#pragma once

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <chrono>

#include <boost/beast/core.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/ip/tcp.hpp>


#include "ws_session.hpp"
#include <abstract_service.hpp>
#include <websocket.hpp>


namespace stream_cloud {
    namespace providers {
        namespace ws_server {

            using tcp = boost::asio::ip::tcp;
            namespace http = boost::beast::http;
            namespace websocket = boost::beast::websocket;


            class ws_listener : public std::enable_shared_from_this<ws_listener> {

            public:
                ws_listener(
                        boost::asio::io_context &ioc,
                        tcp::endpoint endpoint,
                        actor::actor_address,
                        std::initializer_list<actor::actor_address>
                );

                void write(const intrusive_ptr<api::web_socket>& ptr);

                void close(const intrusive_ptr<api::web_socket>& ptr);

                void remove(const intrusive_ptr<api::web_socket>& ptr);

                void close_all();

                void run();

                void do_accept();

                void on_accept(boost::system::error_code ec);

            private:
                boost::asio::strand<boost::asio::io_context::executor_type> strand_;
                tcp::acceptor acceptor_;
                tcp::socket socket_;
                actor::actor_address main_pipe_;
                std::vector<actor::actor_address> pipe_;
                std::unordered_map<api::transport_id,std::shared_ptr<ws_session>> storage_sessions;
            };

        }
    }
}