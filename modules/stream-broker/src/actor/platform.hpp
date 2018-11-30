#pragma once

#include <memory>
#include <actor/basic_actor.hpp>

namespace stream_cloud {
    namespace actor {
        class broker : public basic_async_actor  {
        public:
            broker(environment::abstract_environment * env, const std::string & name, network::multiplexer* multiplexer)
                    : basic_async_actor(env, name),
                      multiplexer_(multiplexer) {
            }

            virtual ~broker() override  = default;

        protected:
            virtual void initialize() override = 0;

            intrusive_ptr<network::multiplexer> multiplexer_;
        };

    }
}