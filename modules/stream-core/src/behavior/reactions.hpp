#pragma once

#include <memory>
#include <unordered_map>
#include "type_action.hpp"
#include "abstract_action.hpp"

namespace stream_cloud {

    namespace behavior {

        class reactions final {
        public:
            using event_type     = type_action;
            using storage        = std::unordered_map<event_type, std::unique_ptr<abstract_action>>;
            using iterator       = storage::iterator ;
            using const_iterator = storage::const_iterator ;

            reactions();

            reactions(const reactions &) = delete;

            reactions &operator=(const reactions &) = delete;

            reactions(reactions &&) = delete;

            reactions &operator=(reactions &&) = delete;

            ~reactions() = default;

            bool add(abstract_action *aa);

            void execute(context &);

            auto begin() -> iterator;

            auto end() -> iterator;

            auto begin() const -> const_iterator;

            auto end() const -> const_iterator;

            auto cbegin() -> const_iterator;

            auto cend() -> const_iterator;

        private:
            storage reactions_;
        };
    }

}