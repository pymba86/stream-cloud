
#include <standard_handlers/sync_contacts.hpp>
#include <iostream>

namespace stream_cloud {


    inline void error(const std::string& __error__){
        std::cerr << "WARNING" << std::endl;
        std::cerr << "Not initialization actor_address type:" << __error__ << std::endl;
        std::cerr << "WARNING" << std::endl;
    }

    sync_contacts::sync_contacts() :  abstract_action("sync_contacts") {}

    void sync_contacts::invoke(behavior::context & context_) {
        auto address = context_.message().body<actor::actor_address>();

        if(address){
            context_->addresses(address);
        } else {
            error(address->name());
        }
    }
}