
#include <standard_handlers/add_channel.hpp>
#include <iostream>
#include <channel/channel.hpp>

namespace stream_cloud {

    inline void error(const std::string &name) {
        std::cerr << "WARNING" << std::endl;
        std::cerr << "Not initialization channel type:" << name << std::endl;
        std::cerr << "WARNING" << std::endl;
    }

    add_channel::add_channel() : abstract_action("add_channel") {}

    void add_channel::invoke(behavior::context &request) {
        auto channel_ = request.message().body<channel::channel>();
        if (channel_) {
            request->channel(channel_);
        } else {
            error(channel_->name());
        }
    }
}