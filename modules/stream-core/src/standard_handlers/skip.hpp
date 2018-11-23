#pragma once

#include <string>
#include <behavior/abstract_action.hpp>

namespace stream_cloud {
    class skip final : public behavior::abstract_action {
    public:
        skip();

        void invoke(behavior::context &) override final;
    };
}