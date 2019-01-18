
#include <error.hpp>

namespace stream_cloud {
    namespace api {
        error::error(transport_id id, transport_type type) : transport_base(type, id) {}

        error::~error() = default;
    }
}