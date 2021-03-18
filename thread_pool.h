
#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <future>
#include <thread>
#include <vector>

namespace mic {

struct ThreadPool {
public:
	explicit ThreadPool(size_t num_workers = std::thread::hardware_concurrency());
	~ThreadPool();

	template<class F, class... Args>
	std::future<std::result_of_t<F(Args...)>> enqueue(F &&func, Args&&... args);

	[[nodiscard]] inline size_t get_num_workers() const { return workers.size(); }
private:
	std::vector<std::thread> workers;
	std::queue<std::packaged_task<void()>> tasks;
	std::mutex mutex;
	std::condition_variable cv;
	bool stopped = false;
};

ThreadPool::ThreadPool(size_t num_workers) {
	workers.reserve(num_workers);
	for (size_t i = 0; i < num_workers; ++i)
		workers.emplace_back([this]() {
			while (true) {
				std::packaged_task<void()> task;
				{
					std::unique_lock lock(mutex);
					cv.wait(lock, [this]() { return stopped || !tasks.empty(); });
					if (stopped && tasks.empty()) return;
					task = std::move(tasks.front());
					tasks.pop();
				}
				task();
			}
		});
}

ThreadPool::~ThreadPool() {
	{
		std::unique_lock lock(mutex);
		stopped = true;
		cv.notify_all();
	}
	for (auto &worker : workers)
		worker.join();
}

template<class F, class... Args>
std::future<std::result_of_t<F(Args...)>> ThreadPool::enqueue(F &&func, Args&&... args) {
	using result_type = std::result_of_t<F(Args...)>;
	auto task = std::packaged_task<result_type()>(
					std::bind(std::forward<F>(func), std::forward<Args>(args)...)
				);
	auto future = task.get_future();

	std::unique_lock lock(mutex);
	if (stopped) throw std::runtime_error("Cannot enqueue task to a stopped ThreadPool");
	tasks.push(std::packaged_task<void()>(std::move(task)));
	cv.notify_one();
	return future;
}

} // namespace mic
