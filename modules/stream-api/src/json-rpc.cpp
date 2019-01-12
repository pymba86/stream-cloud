#include "json-rpc.hpp"

namespace stream_cloud {
    namespace api {
        namespace json_rpc {

            bool contains(const json::json_map &msg, const std::string &key) {
                return msg.find(key) != msg.end();
            }

            bool is_notify(const json::json_map &msg) {
                return !contains(msg, "id");
            }

            bool is_request(const json::json_map &msg) {
                return contains(msg, "method");
            }

            bool is_response(const json::json_map &msg) {
                return !contains(msg, "method") && contains(msg, "id");
            }


            bool parse(const std::string &raw, request_message &request) {

                json::json_map message{json::data{raw}};

                if (!is_request(message)) {
                    return false;
                }

                request.id = message["id"];

                request.method = message["method"].as<std::string>();

                if (contains(message, "param")) {
                    request.params = message["param"];
                } else {
                    request.params = message["params"];
                }

                if (contains(message, "metadata")) {
                    request.metadata = message["metadata"].as<json::json_map>();
                } else {
                    json::json_map metadata;
                    request.metadata = metadata;
                }

                return true;

            }

            bool parse(const json::json_map &message, notify_message &notify) {

                if (!is_notify(message)) {
                    return false;
                }

                notify.method = message["method"].as<std::string>();

                if (contains(message, "param")) {
                    notify.params = message["param"];
                } else {
                    notify.params = message["params"];
                }

                return true;
            }

            bool parse(const json::json_map &message, response_message &response) {

                if (!is_response(message)) {
                    return false;
                }

                response.id = message["id"];
                response.result = message["result"];

                return true;
            }

            std::string serialize(const request_message &msg) {

                json::json_map obj;

                obj["jsonrpc"] = "2.0";
                obj["method"] = msg.method;

                if (!msg.params.is(json::json_value::type::null)) {
                    obj["params"] = msg.params;
                }

                obj["id"] = msg.id;

                return obj.to_string();
            }

            std::string serialize(const response_message &msg) {

                json::json_map obj;

                obj["jsonrpc"] = "2.0";
                obj["result"] = msg.result;

                if (msg.error) {
                    json::json_map error;
                    error["code"] = uint64_t(msg.error->code);
                    error["message"] = msg.error->message;
                    error["data"] = msg.error->data;
                    obj["error"] = error;
                }

                obj["id"] = msg.id;

                return obj.to_string();
            }

            std::string serialize(const notify_message &msg) {

                json::json_map obj;

                obj["jsonrpc"] = "2.0";

                obj["method"] = msg.method;

                if (!msg.params.is(json::json_value::type::null)) {
                    obj["params"] = msg.params;
                }

                return obj.to_string();
            }
        }
    }
}