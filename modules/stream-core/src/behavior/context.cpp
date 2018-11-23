
#include <behavior/context.hpp>

namespace stream_cloud {

    namespace behavior {

        context::~context() {
            if(ptr != nullptr ){
                ptr.release();
            }

        }

        auto context::operator->() noexcept -> context_t * {
            return ptr.get();
        }

        auto context::operator->() const noexcept -> context_t * {
            return ptr.get();
        }

        auto context::operator*() noexcept -> context_t & {
            return *ptr;
        }

        auto context::operator*() const noexcept -> context_t & {
            return *ptr;
        }

        context::context(context_t *ptr, messaging::message &&msg) :
                ptr(ptr),
                msg(std::move(msg)) {

        }

        messaging::message& context::message() {
            return msg; /// TODO hack
        }

        const messaging::message &context::message() const {
            return msg;
        }
    }
}