
#include "managers.hpp"
#include <sqlite_modern_cpp.h>


namespace stream_cloud {

    namespace platform {

        using namespace sqlite;

        class managers::impl final {
        public:
            impl(sqlite::database &db) : db_(db) {};

            ~impl() = default;

        private:
            sqlite::database db_;
        };

        managers::managers(config::config_context_t *ctx) : abstract_service(ctx, "managers") {


            attach(
                    behavior::make_handler("create", [](behavior::context &ctx) -> void {
                        // add_trusted_url - http
                        // Добавляем название, ключ
                        // write - ws
                    })
            );

            attach(
                    behavior::make_handler("remove", [](behavior::context &ctx) -> void {
                        // remove_trusted_url - http
                        // disconnect - managers
                        // Удаляем менеджера
                        // write - ws
                    })
            );

            attach(
                    behavior::make_handler("list", [](behavior::context &ctx) -> void {
                        // Получить список менеджеров у пользователя
                        // write - ws
                    })
            );

            attach(
                    behavior::make_handler("connect", [](behavior::context &ctx) -> void {
                        // Добавляем / Меняем статус в базе данных / Проверяет на наличие
                        // register_manager - router
                    })
            );

            attach(
                    behavior::make_handler("disconnect", [](behavior::context &ctx) -> void {
                        // Удаляем / Меняем статус в базе данных
                        // unregister_manager - router
                    })
            );


        }

        void managers::startup(config::config_context_t *) {

        }

        void managers::shutdown() {

        }
    }
}