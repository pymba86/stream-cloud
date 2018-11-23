#pragma once

#include <thread>
#include <forwards.hpp>

namespace stream_cloud {

    namespace executor {

        class abstract_coordinator {
        public:
            virtual void submit(executable *) = 0;

            virtual void start() = 0;

            virtual ~abstract_coordinator() = default;

            abstract_coordinator(std::size_t num_worker_threads, std::size_t max_throughput_param)
                    : max_throughput_(max_throughput_param),
                      num_workers_(((0 == num_worker_threads) ? std::thread::hardware_concurrency() * 2 - 1 : num_worker_threads)) {
            };

            inline size_t max_throughput() const {
                return max_throughput_;
            }

            inline size_t num_workers() const {
                return num_workers_;
            }

        private:
            std::size_t max_throughput_;
            std::size_t num_workers_;
        };
    }

}