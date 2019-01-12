#pragma once


#include "transport_base.hpp"
#include "json-rpc.hpp"

namespace stream_cloud {
    namespace api {
        struct task final {
            task() = default;

            ~task() = default;

            json_rpc::request_message request;
            transport_id transport_id_;

        };
    }
}