#pragma once

#include <string>

namespace stream_cloud {

    namespace behavior {

        class type_action final {
        public:
            type_action() = default;

            type_action(const type_action &) = default;

            type_action &operator=(const type_action &t)= default;

            type_action(type_action &&) = default;

            type_action &operator=(type_action &&) = default;

            ~type_action() = default;

            template<std::size_t N>
            type_action(const char(&aStr)[N]): body_(aStr) {
                hash_ = std::hash<std::string>{}(body_);
            }

            type_action(const char *, std::size_t);

            type_action(const std::string&);

            bool operator==(const type_action &t) const noexcept;

            auto hash() const noexcept -> std::size_t;

            auto to_string() const -> const std::string&;
        private:
            std::string body_;
            std::size_t hash_;
        };
    }

}

namespace std {
    template<>
    struct hash<stream_cloud::behavior::type_action> {
        inline size_t operator()(const stream_cloud::behavior::type_action &ref) const {
            return ref.hash();
        }
    };
}