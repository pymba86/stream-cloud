
#include "configuration.hpp"

namespace stream_cloud {
    namespace config {

        constexpr const char *config_name_file = "config.json";

        void load_config(config::configuration &config, boost::filesystem::path &config_path) {
            std::cout << "Load config from file: " << config_path.string() << std::endl;
            config.dynamic_configuration = api::json::json_map{api::json::json_file{config_path.string()}};
        }

        void generate_config(config::configuration &config) {
            std::cout << "Load default config" << std::endl;
            config.dynamic_configuration = api::json::json_map{
                    {"http-port", "8080"},
                    {"http-ip", "0.0.0.0"},
                    {"num_worker_threads", 0},
                    {"max_throughput_param_worker", 1000},
                    {"ws-port",   "8081"},
                    {"profile-key",   ""},
                    {"manager-key",   ""},
                    {"db",   ""},
                    {"client",    {
                                          {"ip", "127.0.0.1"},
                                          {"port", "8081"}
                                  }
                    }

            };
        }

        void load_or_generate_config(config::configuration &config, const boost::filesystem::path &path) {

            // set current path
            config.path_app = path;
            boost::filesystem::path config_path = path / config_name_file;

            if (boost::filesystem::exists(config_path)) {
                load_config(config, config_path);
            } else {
                generate_config(config);
            }
        }

    }
}