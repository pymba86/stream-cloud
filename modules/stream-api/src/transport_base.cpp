#include <transport_base.hpp>

namespace stream_cloud {
    namespace api {
        transport_base::transport_base(transport_type type, transport_id id) : type_(type), id_(id) {}

        auto transport_base::type() -> transport_type {
            return type_;
        }

        auto transport_base::id() -> transport_id {
            return id_;
        }

    }
}