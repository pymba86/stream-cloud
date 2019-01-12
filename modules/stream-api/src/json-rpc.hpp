#pragma once


#include <utility>

#include <utility>

#include <utility>
#include <boost/optional.hpp>


#include "json/value.hpp"

namespace stream_cloud {
    namespace api {
        namespace json_rpc {

            enum class type : char {
                non,
                request,
                response,
                error,
                notify
            };

            enum class error_code {
                parse_error = -32700,
                invalid_request = -32600,
                methodNot_found = -32601,
                invalid_params = -32602,
                internal_error = -32603,
                server_error_Start = -32099,
                server_error_end = -32000,
                server_not_initialized,
                unknown_error_code = -32001
            };


            struct request_message final {
                request_message() = default;

                ~request_message() = default;

                request_message(
                        std::string id,
                        std::string method,
                        json::json_value params) :
                        id(std::move(id)),
                        method(std::move(method)),
                        params(std::move(params)) {}

                json::json_value id;
                std::string method;
                json::json_value params;
                json::json_map metadata;
            };

            struct response_error final {
                response_error() = default;

                ~response_error() = default;

                error_code code;
                std::string message;
                json::json_value data;

                response_error(error_code code, std::string message)
                        : code(code), message(std::move(message)) {}

            };

            struct response_message final {
                response_message() = default;

                ~response_message() = default;

                explicit response_message(std::string id, json::json_value result = json::json_value())
                        : id(std::move(id)), result(std::move(result)) {}


                json::json_value id;
                json::json_value result;
                boost::optional<response_error> error;
            };

            struct notify_message final {
                notify_message(std::string method, json::json_value params = json::json_value()) :
                        method(std::move(method)),
                        params(std::move(params)) {}

                std::string method;
                json::json_value params;
            };


            class context final {
            public:
                context() : type_(type::non) {}

                ~context() = default;

                void make_request(
                        std::string id,
                        std::string method,
                        json::json_value params) {
                    assert( type_ != type::non );
                    type_ = type::request;
                    this->id = std::move(id);
                    method_ = std::move(method);
                    this->params = std::move(params);
                }

                void make_notify(std::string method, json::json_value params = json::json_value()) {
                    assert(type_!=type::non);
                    type_ = type::notify;
                    method_ = std::move(method);
                    this->params = std::move(params);
                }

                void make_response(std::string id, json::json_value result = json::json_value()) {
                    assert(type_!=type::non);
                    type_ = type::response;
                    this->id = std::move(id);
                    this->result = std::move(result);
                }

                void make_error(error_code code, std::string message) {
                    assert(type_!=type::non);
                    this->code = code;
                    this->message = std::move(message);
                }

                const std::string& method() const {
                    return method_;
                }


            private:
                type type_;
                json::json_value id;
                std::string method_;
                json::json_value params;
                json::json_value result;
                error_code code;
                std::string message;
                json::json_value data;
            };

            ///Experimental }

            bool parse(const std::string &raw, request_message &request);

            bool parse(const json::json_map &message, notify_message &notify);

            bool parse(const json::json_map &message, response_message &response);

            std::string serialize(const request_message &msg);

            std::string serialize(const response_message &msg);

            std::string serialize(const notify_message &msg);

            bool contains(const json::json_map &msg, const std::string &key);

        }
    }
}
