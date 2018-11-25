#pragma once

#include "transport_base.hpp"
#include <unordered_map>
#include <string>
#include <iostream>

namespace stream_cloud {
    namespace api {
        enum class http_method {
            unknown = 0,
            delete_,
            get,
            head,
            post,
            put,
            connect,
            options,
            trace,
        };

        class http final : public transport_base {
        public:
            using header_storage = std::unordered_map<std::string, std::string>;
            using header_const_iterator = typename  header_storage::const_iterator;
            using header_iterator = typename  header_storage::iterator;

            http() = delete;
            http(const http&) = default;
            http&operator=(const http&) = default;
            http(http&&) = default;
            http&operator=(http&&) = default;

            virtual ~http() override = default;

            http(transport_id);
            ///header
            void header(const char*key,const char* value);
            void header(std::string&&,std::string&&);
            ///header
            ///url
            void uri(const std::string&);
            const std::string& uri() const;
            ///url
            ///method
            void method(const std::string&);
            const std::string& method() const;
            ///method
            ///body
            void body(const std::string&);
            const std::string& body() const;
            ///body
            /// status code
            void status(unsigned code);
            unsigned status() const;
            ///status code

            auto begin() -> header_iterator;
            auto end() -> header_iterator;

        private:
            unsigned status_code;
            std::string uri_;
            std::string method_;
            std::string body_;
            header_storage headers_;

        };

    }
}