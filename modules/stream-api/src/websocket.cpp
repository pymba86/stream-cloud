
#include <websocket.hpp>

namespace stream_cloud {
    namespace api {
        web_socket::web_socket(transport_id id) :transport_base(transport_type::ws,id) {}

        web_socket::~web_socket() = default;
    }
}