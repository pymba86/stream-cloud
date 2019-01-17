#pragma once

#include "transport_base.hpp"
#include <unordered_map>
#include <string>
#include <boost/system/error_code.hpp>

namespace stream_cloud {
    namespace api {
        struct error final  : public  transport_base {
            error(transport_id);
            virtual ~error();
            boost::system::error_code code;

        };
    }
}