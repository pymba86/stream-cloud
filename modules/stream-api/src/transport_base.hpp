#pragma once

#include <memory>
#include <boost/optional.hpp>

namespace stream_cloud {
    namespace api {

        enum class transport_type : unsigned char {
            http = 0x00,
            ws   = 0x01,
        };


        using transport_id = std::string ;

    class transport_base : public std::enable_shared_from_this<transport_base>  {
        public:
            transport_base(transport_type type,transport_id);
            virtual ~transport_base() = default;
            auto type() -> transport_type;
            auto id() -> transport_id;

        public:
            transport_type  type_;
            transport_id    id_;

        };

        using transport = std::shared_ptr<transport_base>;

        template <typename T,typename ...Args>
        inline auto make_transport(Args... args) -> transport {
            return std::shared_ptr<T>(new T (std::forward<Args>(args)...));
        }

    }
}