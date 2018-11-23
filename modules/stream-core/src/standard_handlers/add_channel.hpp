#pragma once


#include <behavior/abstract_action.hpp>

namespace stream_cloud {

    class add_channel final : public behavior::abstract_action {
    public:
        add_channel();

        void invoke(behavior::context &) final;

    };
}