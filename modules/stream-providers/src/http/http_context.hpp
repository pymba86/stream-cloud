#pragma once

#include <boost/beast/http/string_body.hpp>
#include <forward.hpp>
#include <transport_base.hpp>

namespace stream_cloud {
    namespace providers {
        namespace http_server {
            namespace http = boost::beast::http;

            class http_session;

            struct http_context {

                virtual auto check_url(const std::string &) const -> bool = 0;
                virtual auto operator()( http::request <http::string_body>&& , const std::shared_ptr<http_session>& ) -> void = 0;
                virtual ~http_context()= default;

            };
        }
        }
        }