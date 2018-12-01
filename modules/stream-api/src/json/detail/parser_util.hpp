#pragma once

#include <string>
#include <sstream>
#include <iterator>
#include <algorithm>

#include "../map.hpp"
#include "../array.hpp"
#include "../file.hpp"
#include "utf.hpp"


namespace stream_cloud {
    namespace api {
        namespace json {
            namespace detail
            {
                enum class state_t{ parse_name, parse_value };

                template <typename Value, typename Parser, typename T>
                state_t push_back
                        (
                                map<Value, Parser> &m,
                                std::string const &key,
                                T const &t
                        )
                {
                    m.set(key, t);
                    return state_t::parse_name;
                }

                template <typename Value, typename Parser, typename T>
                state_t push_back
                        (
                                array<Value, Parser> &arr,
                                std::string const &,
                                T const &t
                        )
                {
                    arr.push_back(t);
                    return state_t::parse_value;
                }

                template <typename Value, typename Parser>
                map<Value, Parser>& get_map
                        (
                                map<Value, Parser> &m,
                                std::string const &key,
                                typename array<Value, Parser>::index_t const
                        )
                { return m.template get<map<Value, Parser>>(key); }

                template <typename Value, typename Parser>
                map<Value, Parser>& get_map
                        (
                                array<Value, Parser> &arr,
                                std::string const &,
                                typename array<Value, Parser>::index_t const index
                        )
                { return arr.template get<map<Value, Parser>>(index); }

                template <typename Value, typename Parser>
                array<Value, Parser>& get_array
                        (
                                map<Value, Parser> &m,
                                std::string const &key,
                                typename array<Value, Parser>::index_t const
                        )
                { return m.template get<array<Value, Parser>>(key); }

                template <typename Value, typename Parser>
                array<Value, Parser>& get_array
                        (
                                array<Value, Parser> &arr,
                                std::string const &,
                                typename array<Value, Parser>::index_t const index
                        )
                { return arr.template get<array<Value, Parser>>(index); }
            }
        }
    }
}