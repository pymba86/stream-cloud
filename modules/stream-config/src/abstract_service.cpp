
#include "abstract_service.hpp"

#include <context.hpp>

namespace stream_cloud {
    namespace config {
        abstract_service::abstract_service(config_context_t *context,const std::string& name):  actor::basic_async_actor(context->env(),name)/*,state_(service_state::initialized)*/{}

        abstract_service::~abstract_service() = default;
    }
}