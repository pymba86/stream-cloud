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
                class queue final {
                    enum {
                        // Maximum number of responses we will queue
                                limit = 8
                    };

                    // The type-erased, saved work item
                    struct work {
                        virtual ~work() = default;

                        virtual void operator()() = 0;
                    };

                    ws_session &self_;
                    std::vector<std::unique_ptr<work>> items_;

                public:
                    explicit queue(ws_session &self);

                    // Returns `true` if we have reached the queue_vm limit
                    bool is_full() const;

                    // Called when a message finishes sending
                    // Returns `true` if the caller should initiate a read
                    bool on_write();

                    // Called by the HTTP handler to send a response.
                    void operator()(const std::string &msg) {
                        // This holds a work item
                        struct work_impl final : work {
                            ws_session &self_;
                            const std::string msg_;

                            work_impl(
                                    ws_session &self,
                                    std::string msg)
                                    : self_(self), msg_(std::move(msg)) {
                            }

                            void
                            operator()() { // self_.ws_.
                                self_.ws_.async_write(boost::asio::buffer(msg_),
                                                      boost::asio::bind_executor(
                                                              self_.strand_,
                                                              std::bind(
                                                                      &ws_session::on_write,
                                                                      self_.shared_from_this(),
                                                                      std::placeholders::_1,
                                                                      std::placeholders::_2)));
                            }
                        };

                        // Allocate and store the work
                        items_.push_back(boost::make_unique<work_impl>(self_, msg));

                        // If there was no previous work, start this one
                        if (items_.size() == 1)
                            (*items_.front())();
                    }
                };

            public:
                explicit ws_session(
                        tcp::socket,
                        api::transport_id,
                        actor::actor_address
                );

                void run();

                void write(const intrusive_ptr<api::web_socket> &ptr);

                void close();

                void on_accept(boost::system::error_code ec);

                void do_read();

                void on_read(boost::system::error_code ec, std::size_t bytes_transferred);

                void on_write(boost::system::error_code ec, std::size_t bytes_transferred);

                const api::transport_id id_;


            public:

                websocket::stream<tcp::socket> ws_;
                boost::asio::strand<boost::asio::io_context::executor_type> strand_;
                boost::beast::multi_buffer buffer_;
                actor::actor_address pipe_;
                queue queue_;
            };

        }
    }
}