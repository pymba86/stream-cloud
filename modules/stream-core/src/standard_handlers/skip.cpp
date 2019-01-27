#include <standard_handlers/skip.hpp>

#include <iostream>
#include <channel/channel.hpp>

namespace stream_cloud {

    inline void error(const std::string &_error_) {
        std::cerr << "WARNING" << std::endl;
        std::cerr << _error_ << std::endl;
        std::cerr << "WARNING" << std::endl;
    }

    skip::skip() : abstract_action("skip") {}// TODO: "skip" -> "" ?

    void skip::invoke(behavior::context &r) {

        auto sender = r.message()->sender();

        auto command = r.message()->command().to_string();

        if (sender->message_types().count("error")) {
            sender->send(
                    messaging::make_message(
                            r->self(),
                            "error",
                            std::move(command)
                    ));
        } else {
            error(command);
        }
    }
}