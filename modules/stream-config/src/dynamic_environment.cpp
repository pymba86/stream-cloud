#include <dynamic_environment.hpp>
#include <executor/policy/work_sharing.hpp>
#include <executor/coordinator.hpp>
#include <environment/cooperation.hpp>
#include <data_provider.hpp>
#include <environment/group.hpp>

namespace stream_cloud {
    namespace config {

        void startup(data_provider &provider, config_context_t *context) {
            std::cerr << "startup service: " << provider.name() << std::endl;
            ///if (state() == service_state::initialized) {
            ///    state(service_state::started);
            /**    return*/ provider.startup(context);
            ///} else {
            ///    std::cerr << "error startup  service: " << provider.name() << std::endl;
            ///}
        }

        void shutdown(data_provider &provider) {
            std::cerr << "shutdown service:" << provider.name() << std::endl;
            ///if (state() == service_state::started) {
            ///    state(service_state::stopped);
            /**    return*/ provider.shutdown();
            ///} else {
            ///    std::cerr << "error shutdown service:" << provider.name() << std::endl;
            ///}

        }

        struct dynamic_environment::impl final {
            impl() :
                    coordinator_(new executor::coordinator<executor::work_sharing>(1, 1000)),
                    io_serv(new boost::asio::io_service),
                    background_(new boost::thread_group) {
            }

            ~impl() = default;


            auto main_loop() -> boost::asio::io_service * {
                return io_serv.get();
            }

            auto background() const -> boost::thread_group & {
                return *background_;
            }


            environment::cooperation cooperation_;
            std::unique_ptr<executor::abstract_coordinator> coordinator_;

            std::unordered_map<std::string, std::unique_ptr<data_provider> > data_provider_;

        private:

            std::unique_ptr<boost::asio::io_service> io_serv;
            std::unique_ptr<boost::thread_group> background_;

        };

        void dynamic_environment::shutdown() {

            for (auto &i:pimpl->data_provider_) {
                config::shutdown((*i.second.get()));
            }

            pimpl->main_loop()->stop();

        }

        void dynamic_environment::startup() {

            for (auto &i:pimpl->data_provider_) {
                auto provider = i.second.get();
                config::startup(*provider, static_cast<config_context_t *>(this));
            }

            start();

            shutdown();
        }

        void dynamic_environment::initialize() {


        }

        dynamic_environment::dynamic_environment() : pimpl(new impl) {

            std::shared_ptr<boost::asio::signal_set> sigint_set(
                    new boost::asio::signal_set(main_loop(), SIGINT, SIGTERM));
            sigint_set->async_wait(
                    [sigint_set, this](const boost::system::error_code &/*err*/, int /*num*/) {
                        shutdown();
                        sigint_set->cancel();
                    }
            );

        }


        dynamic_environment::~dynamic_environment() {
            pimpl->background().join_all();
            pimpl->main_loop()->stopped();
            std::cerr << "close stream cloud" << std::endl;
        }


        config_context_t *dynamic_environment::context() {
            return static_cast<config_context_t *>(this);
        }

        boost::asio::io_service &dynamic_environment::main_loop() const {
            return *pimpl->main_loop();
        }

        boost::thread_group &dynamic_environment::background() const {
            return pimpl->background();
        }

        int dynamic_environment::start() {
            manager_execution_device().start();
            return pimpl->main_loop()->run();
        }

        executor::abstract_coordinator &dynamic_environment::manager_execution_device() {
            return *pimpl->coordinator_;
        }

       environment::cooperation &dynamic_environment::manager_group() {
            return pimpl->cooperation_;
        }

        auto dynamic_environment::add_service(abstract_service *service_ptr) -> service & {
            return manager_group().created_group(service_ptr);
        }

        auto dynamic_environment::env() -> environment::abstract_environment * {
            return this;
        }

        auto dynamic_environment::add_data_provider(data_provider *ptr) -> data_provider & {
            auto name_ = ptr->name();
            pimpl->data_provider_.emplace(name_, ptr);
            return *(pimpl->data_provider_.at(name_).get());
        }
    }
}