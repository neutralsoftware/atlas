#pragma once

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace atlas::detail {

class Workers {
  public:
    static Workers &shared() {
        static Workers pool;
        return pool;
    }

    std::size_t size() const { return threads.size(); }

    template <class Prepare, class Consume>
    void process(std::size_t count, std::size_t maxInFlight, Prepare prepare,
                 Consume consume,
                 const std::function<void(std::size_t)> &onWait = {}) {
        using Result = std::invoke_result_t<Prepare &, std::size_t>;
        if (count == 0)
            return;
        const auto window = std::clamp(maxInFlight, std::size_t{1}, count);
        std::vector<std::future<Result>> pending(window);
        const auto schedule = [&](std::size_t index) {
            return submit([&, index] { return prepare(index); });
        };

        try {
            for (std::size_t i = 0; i < window; ++i)
                pending[i] = schedule(i);
            for (std::size_t i = 0; i < count; ++i) {
                auto &slot = pending[i % window];
                if (onWait) {
                    while (slot.wait_for(std::chrono::milliseconds(16)) !=
                           std::future_status::ready)
                        onWait(i);
                } else {
                    slot.wait();
                }
                auto completed = std::move(slot);
                if (i + window < count)
                    slot = schedule(i + window);
                consume(i, std::move(completed));
            }
        } catch (...) {
            for (auto &job : pending) {
                if (job.valid())
                    job.wait();
            }
            throw;
        }
    }

    template <class Work>
    void parallelFor(std::size_t count, std::size_t minimumBatch, Work work) {
        const auto batches = (count + minimumBatch - 1) / minimumBatch;
        if (batches <= 1) {
            for (std::size_t i = 0; i < count; ++i)
                work(i);
            return;
        }
        process(batches, size(),
                [&](std::size_t batch) {
                    const auto begin = batch * minimumBatch;
                    const auto end = std::min(count, begin + minimumBatch);
                    for (auto i = begin; i < end; ++i)
                        work(i);
                },
                [](std::size_t, std::future<void> done) { done.get(); });
    }

  private:
    Workers() {
        const auto available = std::thread::hardware_concurrency();
        const auto count = available > 1 ? std::min(available - 1, 12u) : 1u;
        threads.reserve(count);
        for (unsigned int i = 0; i < count; ++i) {
            threads.emplace_back([this](std::stop_token stop) {
                for (;;) {
                    std::function<void()> task;
                    {
                        std::unique_lock lock(mutex);
                        if (!ready.wait(lock, stop, [this] {
                                return !tasks.empty();
                            }))
                            return;
                        task = std::move(tasks.front());
                        tasks.pop_front();
                    }
                    task();
                }
            });
        }
    }

    template <class Work>
    auto submit(Work work) {
        using Result = std::invoke_result_t<Work &>;
        auto task =
            std::make_shared<std::packaged_task<Result()>>(std::move(work));
        auto future = task->get_future();
        {
            std::lock_guard lock(mutex);
            tasks.emplace_back([task = std::move(task)] { (*task)(); });
        }
        ready.notify_one();
        return future;
    }

    std::mutex mutex;
    std::condition_variable_any ready;
    std::deque<std::function<void()>> tasks;
    std::vector<std::jthread> threads;
};

}
