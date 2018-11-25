#pragma once

#include <memory>
#include <boost/optional.hpp>
#include <intrusive_ptr.hpp>

namespace stream_cloud {
    namespace api {

        enum class transport_type : unsigned char {
            http = 0x00,
            ws   = 0x01,
        };


        using transport_id = std::size_t ;

        class transport_base : public intrusive_base<transport_base>  {
        public:
            transport_base(transport_type type,transport_id);
            virtual ~transport_base() = default;
            auto type() -> transport_type;
            auto id() -> transport_id;

        protected:
            transport_type  type_;
            transport_id    id_;

        };

        using transport = intrusive_ptr<transport_base>;

        template <typename T,typename ...Args>
        inline auto make_transport(Args... args) -> transport {
            return intrusive_ptr<T>(new T (std::forward<Args>(args)...));
        }

    }
}