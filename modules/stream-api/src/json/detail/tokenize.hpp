#pragma once

#include <vector>
#include <string>

#include <boost/algorithm/string.hpp>

namespace stream_cloud {
    namespace api {
        namespace json {
            namespace detail
            {
                inline std::vector<std::string> tokenize
                        (
                                std::string const &source,
                                std::string const &delim
                        )
                {
                    std::vector<std::string> tokens;
                    boost::algorithm::split(tokens, source, boost::is_any_of(delim));
                    return tokens;
                }
            }
        }
    }
}