#include <standard_handlers/skip.hpp>

#include <iostream>


namespace stream_cloud {

    inline void error(const std::string& _error_){
        std::cerr << "WARNING" << std::endl;
        std::cerr << _error_ << std::endl;
        std::cerr << "WARNING" << std::endl;
    }

    skip::skip() : abstract_action("skip") {}// TODO: "skip" -> "" ?

    void skip::invoke(behavior::context &r) {
        error(r.message().command().to_string());

    }
}