
#include <behavior/abstract_action.hpp>

namespace stream_cloud {

    namespace behavior {

        abstract_action::~abstract_action() = default;

        auto behavior::abstract_action::name() const -> const type_action & {
            return name_;
        }
    }}
