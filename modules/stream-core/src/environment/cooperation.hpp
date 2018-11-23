#pragma once

#include <actor/abstract_actor.hpp>
#include <unordered_map>
#include "storage_space.hpp"
#include "group.hpp"

namespace stream_cloud {

    namespace environment {

        class cooperation {
        public:
            cooperation() = default;

            cooperation(const cooperation &) = default;

            cooperation &operator=(const cooperation &) = default;

            cooperation(cooperation &&) = default;

            cooperation &operator=(cooperation &&) = default;

            ~cooperation() = default;

            auto created_group(actor::abstract_actor *t) -> group &;

            auto created_entry_group(actor::abstract_actor *t) -> group &;

            auto created_end_group(actor::abstract_actor *t) -> group &;

        private:
            std::unordered_map<std::string, group> groups;
            storage_space storage_space_;
            layer input_entry_point;
            layer output_end_point;
        };

        auto link(group &g1, group &g2) -> void;

    }
}