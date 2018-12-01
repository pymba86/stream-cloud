#pragma once


#include <string>
#include <json/detail/config.hpp>
#include <map>

namespace stream_cloud {
    namespace api {
        namespace json
        {
            /* To customize json internals, change these types
             * to any other compatible types which suit your needs.
             * Add any required #includes here.
             */
            template <>
            struct config<config_tag>
            {
                using float_t = double;
                using int_t = int64_t;

                template <typename K, typename V>
                using map_t = std::map<K, V>;
            };
        }
    }
}