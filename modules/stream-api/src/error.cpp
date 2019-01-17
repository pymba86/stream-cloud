
#include <error.hpp>

namespace stream_cloud {
    namespace api {
        error::error(transport_id id) : transport_base(transport_type::ws, id) {}

        error::~error() = default;
    }
}