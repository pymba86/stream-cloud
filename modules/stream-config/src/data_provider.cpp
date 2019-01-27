#include <data_provider.hpp>

#include <context.hpp>

namespace stream_cloud {
    namespace config {
        bool data_provider::send(std::shared_ptr<messaging::message> message) {
            {
                behavior::context context_(this, std::move(message));
                reactions_.execute(context_); /** context processing */
            }
            return true;

        }

        data_provider::data_provider(config_context_t *context, const std::string &name) : sync_actor(context->env(),
                                                                                                       name) {

        }

        bool data_provider::send(std::shared_ptr<messaging::message> message, executor::execution_device *) {
            {
                behavior::context context_(this, std::move(message));
                reactions_.execute(context_); /** context processing */
            }
        }

        void data_provider::launch(executor::execution_device *, bool) {

        }

        data_provider::~data_provider() = default;
    }
}