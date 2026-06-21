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

		for (size_t i = 0; i < this->ThreadPoolCount; i++)
		{
			std::vector<std::thread> pool{};
			pool.reserve(this->ThreadPoolSize);

			for (uint32_t j = 0; j < this->ThreadPoolSize; j++)
			{
				pool.emplace_back([this, i, j]()
				{
#if defined(_WIN32)
					uint32_t hwThreads = std::thread::hardware_concurrency();
					uint32_t coreCount = hwThreads > 1 ? hwThreads : 1;

					DWORD_PTR mask = (DWORD_PTR(1) << (j % coreCount));

					SetThreadAffinityMask(GetCurrentThread(), mask);

					std::wstring name = L"RC_Clockwork_WorkerThread_" + std::to_wstring(i) + L"_" + std::to_wstring(j);
					SetThreadDescription(GetCurrentThread(), name.c_str());
#elif defined(__linux__)
					throw "NOT IMPLEMENTED";
#endif

					WorkerLoop();
				});
			}

			this->ThreadPools.emplace_back(std::move(pool));
		}
	}

	void Context::QueueTask(ETaskPriority aPriority, std::shared_ptr<ITask> aTask)
	{
		std::lock_guard<std::mutex> lock(this->TaskMutex);

		RC_ASSERT(aPriority < ETaskPriority::COUNT);

		this->TaskQueue[static_cast<uint32_t>(aPriority)].push(aTask);

		this->TaskConVar.notify_one();
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
		this->TaskConVar.notify_all();

		for (size_t i = 0; i < this->ThreadPoolCount; i++)
		{
			for (uint32_t j = 0; j < this->ThreadPoolSize; j++)
			{
				if (this->ThreadPools[i][j].joinable())
				{
					this->ThreadPools[i][j].join();
				}
			}
		}
	}

	void Context::WorkerLoop()
	{
		while (this->IsRunning)
		{
			std::shared_ptr<ITask> task = nullptr;

			{
				std::unique_lock lock(this->TaskMutex);

				this->TaskConVar.wait(lock, [this] {
					return !this->IsRunning
						|| !this->TaskQueue[static_cast<uint32_t>(ETaskPriority::Immediate)].empty()
						|| !this->TaskQueue[static_cast<uint32_t>(ETaskPriority::High)].empty()
						|| !this->TaskQueue[static_cast<uint32_t>(ETaskPriority::Normal)].empty()
						|| !this->TaskQueue[static_cast<uint32_t>(ETaskPriority::Low)].empty();
				});

				if (!this->IsRunning)
				{
					return;
				}

				for (uint32_t i = static_cast<uint32_t>(ETaskPriority::COUNT); i > 0; i--)
				{
					if (this->TaskQueue[i - 1].empty())
					{
						continue;
					}

					task = this->TaskQueue[i - 1].front();

					if (task)
					{
						this->TaskQueue[i - 1].pop();
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
