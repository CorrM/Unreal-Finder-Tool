#pragma once
#include <functional>
#include <thread>
#include <mutex>
#include <utility>
#include <vector>


template <typename VecType>
class ParallelWorker
{
	using Lock = std::lock_guard<std::mutex>;
	using ParallelFunction = std::function<void(VecType&, std::mutex&)>;

	ParallelFunction func;
	std::vector<std::thread> threads;
	std::vector<VecType>& queue;
	int threadsCount;
	size_t curIndex;

public:
	std::mutex GMutex;

	ParallelWorker(std::vector<VecType>& vecQueue, int vecStartIndex, int threadNum, ParallelFunction func);
	void Start();
	void WaitAll();
	static uint32_t GetCpuCores();
private:
	void Worker();
};

template <typename VecType>
ParallelWorker<VecType>::ParallelWorker(std::vector<VecType>& vecQueue, const int vecStartIndex, const int threadNum, ParallelFunction func):
	func(std::move(func)),
	queue(vecQueue),
	threadsCount(threadNum),
	curIndex(vecStartIndex)
{
}

template <typename VecType>
void ParallelWorker<VecType>::Start()
{
	// Create and run threads
	for (int i = 0; i < threadsCount; ++i)
		threads.push_back(std::thread(&ParallelWorker::Worker, this));
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
	while (curIndex < queue.size())
	{
		// Lock Scope (DeQueue item)
		{
			Lock lock(GMutex);

			if (curIndex >= queue.size())
				break;

			curItem = &queue[curIndex];
			++curIndex;
		}

		func(*curItem, GMutex);
	}
}
