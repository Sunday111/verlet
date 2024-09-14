#include "batch_thread_pool.hpp"
#include <ranges>
#include <functional>
#include <algorithm>

namespace verlet
{

BatchThreadPool::BatchThreadPool(size_t threads_count)
    : sync_point_(static_cast<int32_t>(threads_count + 1))
{
    for (const size_t thread_index : std::views::iota(size_t{0}, threads_count))
    {
        threads_.push_back(std::jthread(std::bind_front(&BatchThreadPool::ThreadEntry, this), thread_index));
    }
}

BatchThreadPool::~BatchThreadPool()
{
    std::ranges::for_each(threads_, &std::jthread::request_stop);
    sync_point_.arrive_and_wait();
    std::ranges::for_each(threads_, &std::jthread::join);
}

void BatchThreadPool::ThreadEntry(
    const std::stop_token& stop_token,
    const size_t thread_index)
{
    while (!stop_token.stop_requested())
    {
        sync_point_.arrive_and_wait();
        if (stop_token.stop_requested()) break;
        callback_(context_, thread_index, threads_.size());
        sync_point_.arrive_and_wait();
    }
}

void BatchThreadPool::RunBatch(Callback callback, void* context)
{
    callback_ = callback;
    context_ = context;
    sync_point_.arrive_and_wait();
    sync_point_.arrive_and_wait();
}

}
