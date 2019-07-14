#pragma once
#include <functional>
#include <thread>
#include <mutex>
#include <utility>
#include <vector>

enum class ParallelType
{
	Queue,
	SingleShot
};

struct ParallelOptions
{
	// Current index
	size_t NextIndex;
	// Make this variable true if u want to stop parallel worker
	bool ForceStop;
	// Lock to use with `std::lock_guard`
	std::mutex Locker;
};

/*
 * DON't USE THIS CLASS
 */
template <typename IndexType, typename ItemType>
class ParallelWorker
{
protected:
	IndexType defaultType;
	using Lock = std::lock_guard<std::mutex>;
	using QueueFunc = std::function<void(ItemType&, ParallelOptions&)>;
	using SingleShotFunc = std::function<void(ParallelOptions&)>;

	QueueFunc funcQueue;
	SingleShotFunc funcSingleShot;

	std::vector<std::thread> threads;
	int threadsCount;

	ParallelType type;
	IndexType& queue;

public:
	ParallelOptions Options;

	ParallelWorker(IndexType& indexQueue, size_t startIndex, int threadNum, QueueFunc func);
	ParallelWorker(int threadNum, SingleShotFunc func);
	void Start();
	void WaitAll();
	static uint32_t GetCpuCores();
private:
	void Worker();
};

#pragma region ParallelWorker
// Good for any type that's have Index Operator `[]`, map, vector, array, list, ..etc
template <typename IndexType, typename ItemType>
ParallelWorker<IndexType, ItemType>::ParallelWorker(IndexType& indexQueue, const size_t startIndex, const int threadNum, QueueFunc func) :
	funcQueue(std::move(func)),
	threadsCount(threadNum),
	type(ParallelType::Queue),
	queue(indexQueue)
{
	Options.NextIndex = startIndex;
	Options.ForceStop = false;
}

// Good to use while(true) or something like that
template <typename IndexType, typename ItemType>
ParallelWorker<IndexType, ItemType>::ParallelWorker(const int threadNum, SingleShotFunc func) :
	funcSingleShot(std::move(func)),
	threadsCount(threadNum),
	type(ParallelType::SingleShot),
	queue(defaultType)
{
	Options.NextIndex = 0;
	Options.ForceStop = false;
}

template <typename IndexType, typename ItemType>
void ParallelWorker<IndexType, ItemType>::Start()
{
	Options.ForceStop = false;
	// Create and run threads
	for (int i = 0; i < threadsCount; ++i)
		threads.push_back(std::thread(&ParallelWorker::Worker, this));
}

template <typename IndexType, typename ItemType>
void ParallelWorker<IndexType, ItemType>::WaitAll()
{
	// Create and run threads
	for (int i = 0; i < threadsCount; ++i)
		threads[i].join();
}

template <typename IndexType, typename ItemType>
uint32_t ParallelWorker<IndexType, ItemType>::GetCpuCores()
{
	return std::thread::hardware_concurrency();
}

template <typename IndexType, typename ItemType>
void ParallelWorker<IndexType, ItemType>::Worker()
{
	if (type == ParallelType::Queue)
	{
		ItemType* curItem;
		while (Options.NextIndex < queue.size())
		{
			// Lock Scope (DeQueue item)
			{
				Lock lock(Options.Locker);

				if (Options.NextIndex >= queue.size() || Options.ForceStop)
				{
					Options.ForceStop = false;
					break;
				}

				curItem = &queue[Options.NextIndex];
				++Options.NextIndex;
			}

			funcQueue(*curItem, Options);
		}
	}
	else if (type == ParallelType::SingleShot)
	{
		{
			Lock lock(Options.Locker);
			++Options.NextIndex;
		}

		funcSingleShot(Options);
	}
}
#pragma endregion

#pragma region ParallelQueue
template <typename IndexType, typename ItemType>
class ParallelQueue : public ParallelWorker<IndexType, ItemType>
{
public:
	ParallelQueue(IndexType& indexQueue, size_t startIndex, int threadNum, typename ParallelWorker<IndexType, ItemType>::QueueFunc func);
};

template <typename IndexType, typename ItemType>
ParallelQueue<IndexType, ItemType>::ParallelQueue(IndexType& indexQueue, const size_t startIndex, const int threadNum, typename ParallelWorker<IndexType, ItemType>::QueueFunc func)
	: ParallelWorker<IndexType, ItemType>(indexQueue, startIndex, threadNum, func) { }
#pragma endregion

#pragma region ParallelSingleShot
class ParallelSingleShot : public ParallelWorker<std::vector<nullptr_t>, nullptr_t>
{
public:
	ParallelSingleShot(int threadNum, SingleShotFunc func);
};

inline ParallelSingleShot::ParallelSingleShot(const int threadNum, SingleShotFunc func)
	: ParallelWorker<std::vector<nullptr_t>, nullptr_t>(threadNum, func) { }
#pragma endregion
