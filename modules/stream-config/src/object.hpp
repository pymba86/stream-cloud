#pragma once

#include <map>
#include <dynamic.hpp>

namespace stream_cloud {
    namespace config {

        class dynamic_config::object_t final : public std::map<std::string, dynamic_config> {
            using base_type = std::map<std::string, dynamic_config>;

        public:
            object_t() = default;

            template<class InputIt>
            object_t(InputIt first, InputIt last):
                    base_type(first, last) {}

            object_t(const object_t &other) :
                    base_type(other) {}

            object_t(object_t &&other) :
                    base_type(std::move(other)) {}

            object_t(std::initializer_list<value_type> list) :
                    base_type(list) {}

            object_t(const base_type &other) :
                    base_type(other) {}

            object_t(base_type &&other) :
                    base_type(std::move(other)) {}

            object_t &operator=(const object_t &other) {
                base_type::operator=(other);
                return *this;
            }

            object_t &operator=(object_t &&other) {
                base_type::operator=(std::move(other));
                return *this;
            }

            dynamic_config &at(const std::string &key);

            const dynamic_config &at(const std::string &key) const;

            dynamic_config &at(const std::string &key, dynamic_config &def);

            const dynamic_config &at(const std::string &key, const dynamic_config &def) const;

            using base_type::operator[];

            const dynamic_config &operator[](const std::string &key) const;
        };

    }
}