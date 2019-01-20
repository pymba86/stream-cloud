#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>

#include <abstract_service.hpp>
#include "http_context.hpp"
#include "http_session.hpp"
#include <transport_base.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/config.hpp>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace stream_cloud {
    namespace providers {
        namespace http_server {
            class listener final :
                    public std::enable_shared_from_this<listener>,
                    public http_context {
            public:
                listener(
                        boost::asio::io_context &ioc,
                        tcp::endpoint endpoint,
                        actor::actor_address pipe_,
                        std::shared_ptr<std::string const> const& doc_root
                );

                ~listener() override = default;

                void add_trusted_url(std::string name);

                void remove_trusted_url(std::string name);

                auto check_url(const std::string &) const  -> bool override;

                auto operator()(http::request <http::string_body>&& , const std::shared_ptr<http_session>& ) -> void override;

                std::string path_cat(boost::beast::string_view base, boost::beast::string_view path) const;

                boost::beast::string_view mime_type(boost::beast::string_view path) const;

                void run();

                void do_accept();

                void on_accept(boost::system::error_code ec);

            private:
                tcp::acceptor acceptor_;
                tcp::socket socket_;
                actor::actor_address pipe_;
                std::unordered_set<std::string> trusted_url;
                std::shared_ptr<std::string const> doc_root_;

            };
        }
    }
}