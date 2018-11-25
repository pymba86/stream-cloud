
#include <messaging/message.hpp>
#include <messaging/message_header.hpp>
#include <utility>

namespace stream_cloud {

    namespace messaging {

        auto message::command() const noexcept -> const behavior::type_action & {
            return header_.command();
        }

        auto message::clone() const ->  message {
            return message(header_,body_);
        }

        message::operator bool() {
            return init;
        }

        message::message(actor::actor_address& sender_,const std::string& name, any &&body):
                init(true),
                header_(sender_,name),
                body_(std::move(body)) {}

        message::message(const message_header &header, const any &body):
                init(true),
                header_(header),
                body_(body) {}


        auto message::sender() const -> actor::actor_address {
            return header_.sender();
        }

        message::message():init(false),header_(),body_() {

        }

        void message::swap(message &other) noexcept {
            using std::swap;
            swap(header_, other.header_);
            swap(body_, other.body_);
        }
    }
}