#pragma once


#include "transport_base.hpp"
#include "json-rpc.hpp"
#include <unordered_map>

namespace stream_cloud {
    namespace api {
        struct task final {
            task() = default;

            ~task() = default;

            json_rpc::request_message request;
            transport_id transport_id_;
            std::unordered_map<std::string, std::string> storage;

        };
    }
}