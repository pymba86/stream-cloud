#pragma once

#include <memory>
#include "abstract_environment.hpp"

namespace stream_cloud {

    namespace environment {

        class environment final {
        public:
            environment() = delete;

            environment(const environment &) = default;

            environment &operator=(const environment &)= delete;

            environment(environment &&) = default;

            environment &operator=(environment &&)= default;

            ~environment();

            environment(abstract_environment *ptr);

            template<class T>
            environment &operator=(T *ptr) {
                environment tmp{ptr};
                swap(tmp);
                return *this;
            }

            auto operator->() noexcept -> abstract_environment *;

            explicit operator bool() const noexcept;

            bool operator!() const noexcept;

        private:

            void swap(environment &) noexcept;

            std::unique_ptr<abstract_environment> environment_;
        };

    }

}