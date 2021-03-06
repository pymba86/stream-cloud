#pragma once

#include <metadata.hpp>
#include <set>
#include <forwards.hpp>
#include <environment/environment.hpp>

namespace stream_cloud {
    namespace actor {
    class abstract_actor : public std::enable_shared_from_this<abstract_actor> {
        public:
            virtual bool send(std::shared_ptr<messaging::message>) = 0;

            virtual bool send(std::shared_ptr<messaging::message>, executor::execution_device *) = 0;

            virtual ~abstract_actor() = default;

            actor_address address() const noexcept;

            abstract type() const;

            const std::string &name() const;

            locations locating() const;

            virtual std::set<std::string> message_types() const;

            abstract_actor() = delete;

            abstract_actor(const abstract_actor &) = delete;
        protected:

            environment::environment& env();

            abstract_actor(environment::abstract_environment *, const std::string &);

            metadata type_;

        private:
            abstract_actor &operator=(const abstract_actor &) = delete;

            environment::environment env_;
        };
    }
}