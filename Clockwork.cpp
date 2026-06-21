///----------------------------------------------------------------------------------------------------
/// Copyright (c) Raidcore.GG - All rights reserved.
///
/// Name         :  Context.cpp
/// Description  :  Definition for Clockwork Context singleton class.
/// Authors      :  K. Bieniek
///----------------------------------------------------------------------------------------------------

#include "Clockwork.h"

#include <string>
#include <thread>
#include <windows.h>

namespace Raidcore::Clockwork
{
	static Context* s_Context = nullptr;

	/*static*/ Context* Context::Create(uint32_t aThreadPoolCount, uint32_t aThreadPoolSize)
	{
		RC_ASSERT(!s_Context);
		s_Context = new Context(aThreadPoolCount, aThreadPoolSize);
		s_Context->Init();
		return s_Context;
	}

	/*static*/ Context* Context::Get()
	{
		RC_ASSERT(s_Context);
		return s_Context;
	}
	
	/*static*/ void Context::Destroy()
	{
		RC_ASSERT(s_Context);
		delete s_Context;
		s_Context = nullptr;
	}

	void Context::Init()
	{
		this->ThreadPools.reserve(this->ThreadPoolCount);

		this->IsRunning = true;

		for (uint32_t i = 0; i < this->ThreadPoolCount; i++)
		{
			Threadpool* pool = new Threadpool{};
			pool->Threads.reserve(this->ThreadPoolSize);

			for (uint32_t j = 0; j < this->ThreadPoolSize; j++)
			{
				pool->Threads.emplace_back([this, pool, i, j]()
				{
					this->WorkerSetup(i, j);
					this->WorkerLoop(pool);
				});
			}

			this->ThreadPools.emplace_back(pool);
		}
	}

	void Context::QueueTask(uint32_t aPool, ETaskPriority aPriority, std::shared_ptr<ITask> aTask)
	{
		RC_ASSERT(aPool <= this->ThreadPoolCount);

		Threadpool* pool = this->ThreadPools[aPool];

		std::lock_guard<std::mutex> lock(pool->TaskMutex);

		RC_ASSERT(aPriority < ETaskPriority::COUNT);

		pool->TaskQueue[static_cast<uint32_t>(aPriority)].push(aTask);

		pool->TaskConVar.notify_one();
	}
	
	Context::Context(uint32_t aThreadPoolCount, uint32_t aThreadPoolSize)
	{
		uint32_t threadPools = aThreadPoolCount;

		if (aThreadPoolCount < THREAD_POOL_MIN_COUNT)
		{
			threadPools = THREAD_POOL_MIN_COUNT;
		}

		uint32_t threadPoolSize = aThreadPoolSize;

		if (aThreadPoolSize < THREAD_POOL_MIN_THREAD_COUNT)
		{
			threadPoolSize = std::thread::hardware_concurrency();
		}

		RC_ASSERT(threadPools <= THREAD_POOL_MAX_COUNT);
		RC_ASSERT(threadPoolSize <= THREAD_POOL_MAX_THREAD_COUNT);

		this->ThreadPoolCount = threadPools;
		this->ThreadPoolSize = threadPoolSize;
	}

	Context::~Context()
	{
		if (!this->IsRunning)
		{
			return;
		}

		this->IsRunning = false;

		for (size_t i = 0; i < this->ThreadPoolCount; i++)
		{
			Threadpool* pool = this->ThreadPools[i];
			pool->TaskConVar.notify_all();

			for (uint32_t j = 0; j < this->ThreadPoolSize; j++)
			{
				if (pool->Threads[j].joinable())
				{
					pool->Threads[j].join();
				}
			}

			delete pool;
		}
	}

	void Context::WorkerSetup(uint32_t aPool, uint32_t aThread)
	{
		uint32_t hwThreads = std::thread::hardware_concurrency();
		uint32_t coreCount = hwThreads > 1 ? hwThreads : 1;

		std::wstring name = L"RC_Clockwork_WorkerThread_" + std::to_wstring(aPool) + L"_" + std::to_wstring(aThread);

#if defined(_WIN32)
		DWORD_PTR mask = (DWORD_PTR(1) << (aThread % coreCount));

		SetThreadAffinityMask(GetCurrentThread(), mask);
		SetThreadDescription(GetCurrentThread(), name.c_str());
#elif defined(__linux__)
		throw "NOT IMPLEMENTED";
#endif
	}

	void Context::WorkerLoop(Threadpool* aPool)
	{
		while (this->IsRunning)
		{
			std::shared_ptr<ITask> task = nullptr;

			{
				std::unique_lock lock(aPool->TaskMutex);

				aPool->TaskConVar.wait(lock, [this, &aPool] {
					return !this->IsRunning
						|| !aPool->TaskQueue[static_cast<uint32_t>(ETaskPriority::Immediate)].empty()
						|| !aPool->TaskQueue[static_cast<uint32_t>(ETaskPriority::High)].empty()
						|| !aPool->TaskQueue[static_cast<uint32_t>(ETaskPriority::Normal)].empty()
						|| !aPool->TaskQueue[static_cast<uint32_t>(ETaskPriority::Low)].empty();
				});

				if (!this->IsRunning)
				{
					return;
				}

				for (uint32_t i = static_cast<uint32_t>(ETaskPriority::COUNT); i > 0; i--)
				{
					if (aPool->TaskQueue[i - 1].empty())
					{
						continue;
					}

					task = aPool->TaskQueue[i - 1].front();

					if (task)
					{
						aPool->TaskQueue[i - 1].pop();
						break;
					}
				}
			}

			task->Execute(
				CancellationToken(&task->IsCancelled)
			);
		}
	}
}
