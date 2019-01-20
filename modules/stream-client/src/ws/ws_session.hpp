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
                    std::vector<std::shared_ptr<work>> items_;
                    std::mutex mutex_;

                public:
                    explicit queue(ws_session &self);

                    // Returns `true` if we have reached the queue_vm limit
                    bool is_full() const;

                    // Called when a message finishes sending
                    // Returns `true` if the caller should initiate a read
                    bool on_write();

                    void pop_front() {
                        std::lock_guard<std::mutex> lock{mutex_};
                        items_.erase(items_.begin());
                    };

                    std::shared_ptr<work>& front() {
                        std::lock_guard<std::mutex> lock{mutex_};
                        return items_.front();
                    }

                    bool empty() {
                        std::lock_guard<std::mutex> lock{mutex_};
                        return items_.empty();
                    };

                    // Called by the HTTP handler to send a response.
                    void operator()(std::shared_ptr<std::string const> const &msg) {
                        // This holds a work item
                        struct work_impl final : work {
                            ws_session &self_;
                            std::shared_ptr<std::string const> msg_;

                            work_impl(
                                    ws_session &self,
                                    std::shared_ptr<std::string const> msg)
                                    : self_(self), msg_(std::move(msg)) {
                            }

                            void
                            operator()() { // self_.ws_.
                                self_.ws_.async_write(
                                        boost::asio::buffer(std::string(*msg_)),
                                        [sp = self_.shared_from_this()](
                                                boost::system::error_code ec, std::size_t bytes)
                                        {
                                            sp->on_write(ec, bytes);
                                        });
                            }
                        };

                        std::lock_guard<std::mutex> lock{mutex_};

                        // Allocate and store the work
                        items_.push_back(std::make_shared<work_impl>(self_, msg));

                        // If there was no previous work, start this one
                        if (items_.size() > 1)
                            return;

                        (*items_.front())();

                    }
                };

            public:
                explicit ws_session(boost::asio::io_context &ioc,
                                    api::transport_id id,
                                    actor::actor_address pipe_
                );

                void run(const std::string &host_, const std::string &text_);

                void write(const intrusive_ptr<api::web_socket> &ptr);

                void write(const std::string &value);

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
                boost::beast::flat_buffer buffer_;
                actor::actor_address pipe_;
                queue queue_;
                std::string host_;
                std::string port_;
            };


        }
    }
}