#pragma once

#include <context.hpp>
#include <actor/actor_address.hpp>
#include <forward.hpp>
#include "abstract_service.hpp"
#include "configuration.hpp"

namespace stream_cloud {
    namespace config {
        class dynamic_environment final :
                public config_context_t,
                public abstract_environment {
        public:

            explicit dynamic_environment(configuration&&);

            ~dynamic_environment() override;

            template <typename SERVICE,typename ...Args>
            auto add_service(Args &&...args) -> service& {
                return add_service(new SERVICE(static_cast<config_context_t*>(this),std::forward<Args>(args)...));
            }

            template<typename DataProvider,typename ...Args>
            auto add_data_provider(actor::actor_address address,Args &&...args) -> data_provider& {
                return add_data_provider(new DataProvider(static_cast<config_context_t*>(this),std::move(address),std::forward<Args>(args)...));
            }


            void initialize();

            void startup();

            void shutdown();

        private:

            auto env() -> environment::abstract_environment *;

            auto add_service(abstract_service*) ->  service &;

            auto add_data_provider(data_provider*)-> data_provider&;

            auto  config()  const -> api::json::json_map&;

            int start() override ;

            auto manager_execution_device() -> executor::abstract_coordinator & override ;

            auto manager_group() -> environment::cooperation & override ;

            auto context() -> config_context_t *;

            boost::asio::io_service& main_loop() const override;

            boost::thread_group& background() const override;

            struct impl;

            std::unique_ptr<impl> pimpl;
        };

    }
}