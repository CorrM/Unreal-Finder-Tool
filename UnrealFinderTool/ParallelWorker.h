#pragma once
#include <functional>
#include <thread>
#include <mutex>
#include <utility>
#include <vector>
#include "UnsortedMap.h"

enum class ParallelType
{
	Queue,
	SingleShot
};

struct ParallelOptions
{
	std::mutex Locker;
	bool ForceStop;
};

template <typename VecType>
class ParallelWorker
{
	using Lock = std::lock_guard<std::mutex>;
	using QueueFunc = std::function<void(VecType&, ParallelOptions&)>;
	using SingleShotFunc = std::function<void(ParallelOptions&)>;

	QueueFunc funcQueue;
	SingleShotFunc funcSingleShot;

	std::vector<std::thread> threads;
	int threadsCount;

	ParallelType type;

	size_t curIndex;
	std::vector<VecType>& queue;

public:
	ParallelOptions Options{};

	ParallelWorker(std::vector<VecType>& vecQueue, int vecStartIndex, int threadNum, QueueFunc func);
	ParallelWorker(int threadNum, SingleShotFunc func);
	void Start();
	void WaitAll();
	static uint32_t GetCpuCores();
private:
	void Worker();
};

template <typename VecType>
ParallelWorker<VecType>::ParallelWorker(std::vector<VecType>& vecQueue, const int vecStartIndex, const int threadNum, QueueFunc func) :
	funcQueue(std::move(func)),
	threadsCount(threadNum),
	type(ParallelType::Queue),
	curIndex(vecStartIndex),
	queue(vecQueue)
{
}

// Go to use while(true) or something like that
template <typename NullType>
ParallelWorker<NullType>::ParallelWorker(const int threadNum, SingleShotFunc func) :
	funcSingleShot(std::move(func)),
	threadsCount(threadNum),
	type(ParallelType::SingleShot),
	curIndex(0),
	queue(std::vector<NullType>{})
{
}

template <typename VecType>
void ParallelWorker<VecType>::Start()
{
	Options.ForceStop = false;
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
	if (type == ParallelType::Queue)
	{
		VecType* curItem;
		while (curIndex < queue.size())
		{
			// Lock Scope (DeQueue item)
			{
				Lock lock(Options.Locker);

				if (curIndex >= queue.size() || Options.ForceStop)
				{
					Options.ForceStop = true;
					break;
				}

				curItem = &queue[curIndex];
				++curIndex;
			}

			funcQueue(*curItem, Options);
		}
	}
	else if (type == ParallelType::SingleShot)
	{
		funcSingleShot(Options);
	}
}
