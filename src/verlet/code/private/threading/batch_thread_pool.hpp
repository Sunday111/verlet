#pragma once

#include <barrier>
#include <thread>
#include <vector>

namespace verlet
{
class BatchThreadPool
{
public:
    explicit BatchThreadPool(size_t threads_count);
    BatchThreadPool(const BatchThreadPool&) = delete;
    BatchThreadPool(BatchThreadPool&&) = delete;
    ~BatchThreadPool();
    using Callback = void (*)(void* context, size_t thread_index, size_t num_threads);

    [[nodiscard]] size_t GetThreadsCount() const { return threads_.size(); }

    template <std::invocable<size_t, size_t> T>
    void RunBatch(T&& callback)
    {
        RunBatch(
            [](void* context, size_t thread_index, size_t num_threads)
            {
                auto& callback = *reinterpret_cast<T*>(context);
                callback(thread_index, num_threads);
            },
            &callback);
    }

    void RunBatch(Callback callback, void* context);

private:
    void ThreadEntry(const std::stop_token& stop_token, const size_t thread_index);

    std::barrier<> sync_point_;
    std::vector<std::jthread> threads_;
    Callback callback_ = nullptr;
    void* context_ = nullptr;
};
}  // namespace verlet
