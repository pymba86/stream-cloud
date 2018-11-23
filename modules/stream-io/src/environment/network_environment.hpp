#pragma once

#include <environment/abstract_environment.hpp>
#include <environment/cooperation.hpp>
#include <network/multiplexer.hpp>
#include <executor/abstract_coordinator.hpp>

namespace stream_cloud {
    namespace environment {

        using network::multiplexer;
        using executor::abstract_coordinator;

        class network_environment final :
                public abstract_environment {
        public:
            network_environment() = delete;

            network_environment(const network_environment &) = delete;

            network_environment &operator=(const network_environment &) = delete;

            ~network_environment() = default;

            network_environment(abstract_coordinator *coordinator, multiplexer *ptr) :
                    multiplexer_(ptr),
                    coordinator_(coordinator) {

            }

            int start() override final {
                coordinator_->start();
                multiplexer_->start();
                return 0;
            }

            executor::abstract_coordinator &manager_execution_device() override final {
                return *coordinator_.get();
            }

            cooperation &manager_group() override final {
                return cooperation_;
            }


        private:
            intrusive_ptr<multiplexer> multiplexer_;
            cooperation cooperation_;
            std::unique_ptr<abstract_coordinator> coordinator_;
        };
    }
}