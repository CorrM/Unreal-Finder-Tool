#pragma once
#include <functional>
#include <thread>
#include <mutex>
#include <vector>

using Lock = std::lock_guard<std::mutex>;

template <typename VecType>
class ParallelWorker
{
	std::function<void(VecType)> func;
	std::mutex gMutex;
	std::vector<std::thread> threads;
	std::vector<VecType>& queue;
	int threadsCount, curIndex;

public:
	ParallelWorker(std::vector<VecType>& vecQueue, int threadNum, std::function<void(VecType&)> func);
	void Start();
	void WaitAll();

private:
	uint32_t GetCpuCores();
	void Worker();
};

template <typename VecType>
ParallelWorker<VecType>::ParallelWorker(std::vector<VecType>& vecQueue, const int threadNum, const std::function<void(VecType&)> func):
	func(func),
	queue(vecQueue),
	threadsCount(threadNum),
	curIndex(0)
{
}

template <typename VecType>
void ParallelWorker<VecType>::Start()
{
	// Create and run threads
	for (int i = 0; i < threadsCount; ++i)
		threads.emplace_back(Worker);
}

template <typename VecType>
void ParallelWorker<VecType>::WaitAll()
{
	// Create and run threads
	for (int i = 0; i < threadsCount; ++i)
		threads[i].join();
}

template <typename VecType>
uint32_t ParallelWorker<VecType>::GetCpuCores()
{
	return std::thread::hardware_concurrency();
}

template <typename VecType>
void ParallelWorker<VecType>::Worker()
{
	VecType* curItem;
	while (curIndex >= queue.size())
	{
		// Lock Scope (DeQueue item)
		{
			Lock lock(gMutex);
			curItem = &queue[curIndex];
			++curItem;
		}

		func(*curItem);
	}
}
