#pragma once

#include <boost/variant.hpp>

#include "detail/normalize.hpp"
#include "file.hpp"
#include "data.hpp"
#include "map.hpp"
#include "array.hpp"
#include "detail/parser.hpp"
#include "detail/escape.hpp"


namespace stream_cloud {
    namespace api {
        namespace json {
            class value {
            public:
                /* Maps to the variant 1:1. */
                enum class type {
                    null,
                    integer,
                    real,
                    boolean,
                    string,
                    map,
                    array
                };

                using map_t = map<value, detail::parser>;
                using array_t = array<value, detail::parser>;

                struct null_t {
                    bool operator==(null_t const &) const { return true; }

                    bool operator!=(null_t const &) const { return false; }
                };

                using variant_t = boost::variant
                        <
                                null_t,
                                detail::int_t,
                                detail::float_t,
                                bool,
                                std::string,
                                map_t,
                                array_t
                        >;

                template<type T>
                struct to_type;
                template<typename T, typename E = void>
                struct to_value;

                value()
                        : value_{null_t{}} {}

                value(value const &copy)
                        : value_{copy.value_} {}

                template
                        <
                                typename T,
                                typename E = std::enable_if_t<detail::is_convertible<T, value>()>
                        >
                value(T &&val)
                        : value_{null_t{}} { set(std::forward<T>(val)); }

                value
                        (
                                std::initializer_list
                                        <
                                                std::pair<typename map_t::key_t const, value>
                                        > const &list
                        )
                        : value_{null_t{}} { set(map_t{list}); }

                template
                        <
                                typename T,
                                typename E = std::enable_if_t<detail::is_convertible<T, value>()>
                        >
                value(std::initializer_list<T> const &list)
                        : value_{null_t{}} { set(array_t{list}); }

                template<typename T>
                auto &get() { return boost::get<detail::normalize<T>>(value_); }

                template<typename T>
                auto const &get() const { return boost::get<detail::normalize<T>>(value_); }

                template<typename T>
                auto &as() { return get<T>(); }

                template<typename T>
                auto const &as() const { return get<T>(); }

                /* Convenient, but not as type-safe nor performant. */
                value &operator[](map_t::key_t const &key) {
                    if (get_type() != type::map) {
                        throw std::runtime_error
                                {
                                        "invalid value type (" +
                                        std::to_string(value_.which()) +
                                        "); required map"
                                };
                    }
                    return as<map_t>()[key];
                }

                value &operator[](array_t::index_t const &index) {
                    if (get_type() != type::array) {
                        throw std::runtime_error
                                {
                                        "invalid value type (" +
                                        std::to_string(value_.which()) +
                                        "); required array"
                                };
                    }
                    return as<array_t>()[index];
                }

                template<typename T>
                explicit operator T() { return as<T>(); }

                template<typename T>
                explicit operator T() const { return as<T>(); }

                /* TODO: Rename to type() */
                type get_type() const { return static_cast<type>(value_.which()); }

                bool is(type const t) const { return get_type() == t; }

                friend bool operator==(value const &jv, value const &val);

                template<typename T>
                friend bool operator==(value const &jv, T const &val);

                template<typename T>
                friend bool operator==(T const &val, value const &jv);

                friend bool operator!=(value const &jv, value const &val);

                template<typename T>
                friend bool operator!=(value const &jv, T const &val);

                template<typename T>
                friend bool operator!=(T const &val, value const &jv);

                friend std::ostream &operator<<(std::ostream &stream, value const &val);

                /* We can avoid superfluous copying by checking whether or not to normalize. */
                template<typename T>
                std::enable_if_t<!detail::should_normalize<T>()> set(T const &val) { value_ = val; }

                template<typename T>
                std::enable_if_t<detail::should_normalize<T>()> set(T const &val) {
                    value_ = detail::normalize<T>(val);
                }

                void set(std::nullptr_t) { value_ = null_t{}; }

                /* Shortcut add for arrays. */
                template<typename T>
                void push_back(T const &val) { as<array_t>().push_back(val); }

                /* Shortcut add for maps. */
                template<typename T>
                void push_back(std::string const &key, T const &val) { as<map_t>().set(key, val); }

                template
                        <
                                typename T,
                                typename E = std::enable_if_t<detail::is_convertible<T, value>()>
                        >
                variant_t &operator=(T const &val) {
                    set(val);
                    return value_;
                }

            private:
                variant_t value_;
            };

            using map_t = map<value, detail::parser>;
            using array_t = array<value, detail::parser>;

            template<>
            inline auto &value::get<value>() { return *this; }

            template<>
            inline auto const &value::get<value>() const { return *this; }

            template<>
            inline auto &value::as<value>() { return get<value>(); }

            template<>
            inline auto const &value::as<value>() const { return get<value>(); }

            template<>
            struct json::value::to_type<json::value::type::null> {
                using type = value::null_t;
            };
            template<>
            struct json::value::to_type<json::value::type::integer> {
                using type = detail::int_t;
            };
            template<>
            struct json::value::to_type<json::value::type::real> {
                using type = detail::float_t;
            };
            template<>
            struct json::value::to_type<value::type::boolean> {
                using type = bool;
            };
            template<>
            struct json::value::to_type<value::type::string> {
                using type = std::string;
            };
            template<>
            struct json::value::to_type<value::type::map> {
                using type = map_t;
            };
            template<>
            struct json::value::to_type<value::type::array> {
                using type = array_t;
            };

            template<>
            struct json::value::to_value<value::null_t> {
                static type constexpr const value{json::value::type::null};
            };
            template<typename T>
            struct json::value::to_value
                    <
                            T,
                            std::enable_if_t
                                    <
                                            std::is_integral<std::decay_t<T>>
                                            ::value &&
                                            !std::is_same<std::decay_t<T>, bool>::value
                                    >
                    > {
                static type constexpr const value{json::value::type::integer};
            };
            template<typename T>
            struct json::value::to_value
                    <
                            T,
                            std::enable_if_t
                                    <
                                            std::is_floating_point<std::decay_t<T>>
                                            ::value
                                    >
                    > {
                static type constexpr const value{json::value::type::real};
            };
            template<>
            struct json::value::to_value<bool> {
                static type constexpr const value{json::value::type::boolean};
            };
            template<typename T>
            struct json::value::to_value
                    <
                            T,
                            std::enable_if_t
                                    <
                                            json::detail::is_string<std::decay_t<T>>()
                                    >
                    > {
                static type constexpr const value{json::value::type::string};
            };
            template<>
            struct json::value::to_value<json::map_t> {
                static type constexpr const value{json::value::type::map};
            };
            template<>
            struct json::value::to_value<json::array_t> {
                static type constexpr const value{json::value::type::array};
            };

            inline std::ostream &operator<<(std::ostream &stream, json::value const &val) {
                switch (static_cast<value::type>(val.value_.which())) {
                    case json::value::type::string:
                        return (stream << "\"" << json::detail::escape(val.as<std::string>()) << "\"");
                    case json::value::type::boolean:
                        return (stream << (val.as<bool>() ? "true" : "false"));
                    default:
                        return (stream << val.value_);
                }
            }

            template<typename Iter>
            inline void streamjoin
                    (
                            Iter begin, Iter const end, std::ostream &stream,
                            std::string const &sep = ","
                    ) {
                if (begin != end) { stream << *begin++; }
                while (begin != end) { stream << sep << *begin++; }
            }

            inline std::ostream &operator<<(std::ostream &stream, json::value::null_t const &) {
                return (stream << "null");
            }

            template<typename Stream_Value, typename Stream_Parser>
            std::ostream &operator<<
                    (
                            std::ostream &stream,
                            json::array<Stream_Value, Stream_Parser> const &arr
                    );

            template<>
            inline std::ostream &operator<<(std::ostream &stream, json::array_t const &arr) {
                stream << arr.delim_open;
                streamjoin(arr.values_.begin(), arr.values_.end(), stream);
                stream << arr.delim_close;
                return stream;
            }

            inline std::ostream &operator<<
                    (
                            std::ostream &stream,
                            map_t::internal_map_t::value_type const &p
                    ) { return (stream << "\"" << json::detail::escape(p.first) << "\":" << p.second); }

            template<typename Stream_Value, typename Stream_Parser>
            std::ostream &operator<<
                    (
                            std::ostream &stream,
                            map<Stream_Value, Stream_Parser> const &m
                    );

            template<>
            inline std::ostream &operator<<(std::ostream &stream, json::map_t const &m) {
                stream << m.delim_open;
                streamjoin(m.values_.begin(), m.values_.end(), stream);
                stream << m.delim_close;
                return stream;
            }

            using json_value = json::value;
            using json_map = json::map_t;
            using json_array = json::array_t;
            using json_null = json_value::null_t;
            using json_int = json::detail::int_t;
            using json_float = json::detail::float_t;
            using json_file = json::file;
            using json_data = json::data;


            inline bool operator==(json_value const &jv, json_value const &val) {
                return jv.get_type() == val.get_type() && jv.value_ == val.value_;
            }

            template<typename T>
            bool operator==(json_value const &jv, T const &val) {
                return jv.get_type() == value::to_value<T>::value && jv.as<T>() == val;
            }

            template<typename T>
            bool operator==(T const &val, json_value const &jv) { return jv == val; }

            inline bool operator!=(json_value const &jv, json_value const &val) { return !(jv == val); }

            template<typename T>
            bool operator!=(json_value const &jv, T const &val) { return !(jv == val); }

            template<typename T>
            bool operator!=(T const &val, json_value const &jv) { return !(jv == val); }
        }
    }
}