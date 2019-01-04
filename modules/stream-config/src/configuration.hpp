#pragma once

#include <boost/filesystem.hpp>
#include "json/value.hpp"

namespace stream_cloud {
    namespace config {

        struct configuration final {
            api::json::json_map dynamic_configuration;
            boost::filesystem::path         path_app;
        };

        void load_config(config::configuration &config, boost::filesystem::path &config_path);

        void generate_config(config::configuration &config);

        void load_or_generate_config(config::configuration &config);
    }
}
