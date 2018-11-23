#pragma once


#include <behavior/abstract_action.hpp>

namespace stream_cloud {

    class sync_contacts final : public behavior::abstract_action {
    public:
        sync_contacts();

        void invoke(behavior::context &) override final;

    };
}