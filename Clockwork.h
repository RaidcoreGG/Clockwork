///----------------------------------------------------------------------------------------------------
/// Copyright (c) Raidcore.GG - All rights reserved.
///
/// Name         :  Clockwork.h
/// Description  :  Definition for Clockwork context and utility functions.
/// Authors      :  K. Bieniek
///----------------------------------------------------------------------------------------------------

#pragma once

#include <assert.h>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>
#include <windows.h>

#include "Tasks/ETaskPriority.h"
#include "Tasks/ScheduledTask.h"
#include "Tasks/Task.h"
#include "Tasks/TaskBase.h"

///----------------------------------------------------------------------------------------------------
/// Raidcore::Clockwork Namespace
///----------------------------------------------------------------------------------------------------
namespace Raidcore::Clockwork
{
#ifndef RC_ASSERT
#define RC_ASSERT(expression) assert(expression)
#endif

#define THREAD_POOL_MIN_COUNT 1u
#define THREAD_POOL_MIN_THREAD_COUNT 1u

#ifndef THREAD_POOL_MAX_COUNT
#define THREAD_POOL_MAX_COUNT 16u
#endif

#ifndef THREAD_POOL_MAX_THREAD_COUNT
#define THREAD_POOL_MAX_THREAD_COUNT 64u
#endif

	///----------------------------------------------------------------------------------------------------
	/// Context Singleton Class
	///----------------------------------------------------------------------------------------------------
	class Context
	{
		///----------------------------------------------------------------------------------------------------
		/// Threadpool Struct
		///----------------------------------------------------------------------------------------------------
		struct Threadpool
		{
			std::vector<std::thread>                    Threads{};

			std::mutex                                  TaskMutex{};
			std::condition_variable                     TaskConVar{};
			std::queue<std::shared_ptr<ITask>>          TaskQueue[static_cast<uint32_t>(ETaskPriority::COUNT)]{};

			///----------------------------------------------------------------------------------------------------
			/// ScheduledTaskCompare Struct
			///----------------------------------------------------------------------------------------------------
			struct ScheduledTaskCompare
			{
				bool operator()(const std::shared_ptr<ScheduledTask>& a, const std::shared_ptr<ScheduledTask>& b) const
				{
					return a->GetNextExecution() > b->GetNextExecution();
				}
			};

			std::thread                                 SchedulerThread{};
			std::mutex                                  SchedulerMutex{};
			std::condition_variable                     SchedulerConVar{};
			std::priority_queue<
				std::shared_ptr<ScheduledTask>,
				std::vector<std::shared_ptr<ScheduledTask>>,
				ScheduledTaskCompare
			> ScheduledTasks;
		};

		public:
		///----------------------------------------------------------------------------------------------------
		/// Create:
		/// 	Creates the Clockwork context with the specified amounts of threadpools and threads per.
		///----------------------------------------------------------------------------------------------------
		inline static Context* Create(uint32_t aThreadPoolCount = 1, uint32_t aThreadPoolSize = 0)
		{
			RC_ASSERT(!s_Context);
			s_Context = new Context(aThreadPoolCount, aThreadPoolSize);
			s_Context->Init();
			return s_Context;
		}

		///----------------------------------------------------------------------------------------------------
		/// Get:
		/// 	Returns the Clockwork context.
		///----------------------------------------------------------------------------------------------------
		inline static Context* Get()
		{
			RC_ASSERT(s_Context);
			return s_Context;
		}

		///----------------------------------------------------------------------------------------------------
		/// Destroy:
		/// 	Destroys the Clockwork context.
		///----------------------------------------------------------------------------------------------------
		inline static void Destroy()
		{
			RC_ASSERT(s_Context);
			delete s_Context;
			s_Context = nullptr;
		}

		///----------------------------------------------------------------------------------------------------
		/// Init:
		/// 	Initializes the threadpools.
		///----------------------------------------------------------------------------------------------------
		inline void Init()
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

		///----------------------------------------------------------------------------------------------------
		/// QueueTask:
		/// 	Queues a task to the threadpool, waiting for completion.
		///----------------------------------------------------------------------------------------------------
		inline void QueueTask(uint32_t aPool, ETaskPriority aPriority, std::shared_ptr<ITask> aTask)
		{
			RC_ASSERT(aPool <= this->ThreadPoolCount);
			Threadpool* pool = this->ThreadPools[aPool];

			RC_ASSERT(aPriority < ETaskPriority::COUNT);

#pragma warning( push )
#pragma warning( disable : 33011 )
			{
				std::lock_guard<std::mutex> lock(pool->TaskMutex);
				pool->TaskQueue[static_cast<uint32_t>(aPriority)].push(aTask);
			}
#pragma warning( pop )

			pool->TaskConVar.notify_one();
		}

		///----------------------------------------------------------------------------------------------------
		/// Schedule:
		/// 	Schedules a recurring action.
		///----------------------------------------------------------------------------------------------------
		inline void Schedule(uint32_t aPool, std::shared_ptr<ScheduledTask> aTask)
		{
			RC_ASSERT(aPool <= this->ThreadPoolCount);
			Threadpool* pool = this->ThreadPools[aPool];

			{
				std::lock_guard<std::mutex> lock(pool->SchedulerMutex);

				if (pool->ScheduledTasks.size() == 0)
				{
					/* Start scheduler thread. */
					pool->SchedulerThread = std::thread([this, pool, aPool]()
					{
						this->SchedulerSetup(aPool);
						this->SchedulerLoop(pool);
					});
				}

				pool->ScheduledTasks.push(std::move(aTask));
			}

			pool->SchedulerConVar.notify_one();
		}
		
		Context(Context const&) = delete;
		void operator=(Context const&) = delete;

		private:
		inline static Context* s_Context = nullptr;

		bool                     IsRunning{ false };
		uint32_t                 ThreadPoolCount{ 1 };
		uint32_t                 ThreadPoolSize{ 1 };
		std::vector<Threadpool*> ThreadPools{};

		///----------------------------------------------------------------------------------------------------
		/// ctor
		///----------------------------------------------------------------------------------------------------
		inline Context(uint32_t aThreadPoolCount, uint32_t aThreadPoolSize)
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

		///----------------------------------------------------------------------------------------------------
		/// dtor
		///----------------------------------------------------------------------------------------------------
		inline ~Context()
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

				if (pool->SchedulerThread.joinable())
				{
					pool->SchedulerThread.join();
				}

				delete pool;
			}
		}

		///----------------------------------------------------------------------------------------------------
		/// WorkerSetup:
		/// 	Setup function for the worker threads.
		///----------------------------------------------------------------------------------------------------
		inline void WorkerSetup(uint32_t aPool, uint32_t aThread)
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

		///----------------------------------------------------------------------------------------------------
		/// WorkerLoop:
		/// 	Loop function for the worker threads.
		///----------------------------------------------------------------------------------------------------
		inline void WorkerLoop(Threadpool* aPool)
		{
			while (this->IsRunning)
			{
				std::shared_ptr<ITask> task = nullptr;

				{
					std::unique_lock lock(aPool->TaskMutex);

					aPool->TaskConVar.wait(lock, [this, &aPool]
					{
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

				if (task)
				{
					task->Execute(
						CancellationToken(&task->IsCancelled)
					);
				}
			}
		}

		///----------------------------------------------------------------------------------------------------
		/// SchedulerSetup:
		/// 	Setup function for the schedculer threads.
		///----------------------------------------------------------------------------------------------------
		inline void SchedulerSetup(uint32_t aPool)
		{
			std::wstring name = L"RC_Clockwork_SchedulerThread_" + std::to_wstring(aPool);

#if defined(_WIN32)

			SetThreadDescription(GetCurrentThread(), name.c_str());
#elif defined(__linux__)
			throw "NOT IMPLEMENTED";
#endif
		}

		///----------------------------------------------------------------------------------------------------
		///	SchedulerLoop:
		/// 	Loop function for the scheduler threads.
		///----------------------------------------------------------------------------------------------------
		inline void SchedulerLoop(Threadpool* aPool)
		{
			while (this->IsRunning)
			{
				std::unique_lock lock(aPool->SchedulerMutex);

				aPool->SchedulerConVar.wait(lock, [this, &aPool]
				{
					return !this->IsRunning || !aPool->ScheduledTasks.empty();
				});

				if (!this->IsRunning)
				{
					return;
				}

				while (!aPool->ScheduledTasks.empty())
				{
					std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
					std::shared_ptr<ScheduledTask> task = aPool->ScheduledTasks.top();

					if (task->GetNextExecution() > now)
					{
						aPool->SchedulerConVar.wait_until(lock, task->GetNextExecution());
						break;
					}

					aPool->ScheduledTasks.pop();

					/* If the task isn't cancelled, reschedule and execute it. */
					if (!task->IsCancelled.load())
					{
						/* Reschedule for next execution. */
						task->Reschedule();
						aPool->ScheduledTasks.push(task);

						/* Push to worker queue. */
						{
							std::lock_guard taskLock(aPool->TaskMutex);
							aPool->TaskQueue[static_cast<uint32_t>(ETaskPriority::Immediate)].push(task);
						}

						aPool->TaskConVar.notify_one();
					}
				}
			}
		}
	};

	///----------------------------------------------------------------------------------------------------
	/// Run:
	/// 	Runs an action asynchronously and returns a Task object associated with it, that can be awaited.
	///----------------------------------------------------------------------------------------------------
	template <typename T>
	std::shared_ptr<Task<T>> Run(uint32_t aPool, ETaskPriority aPriority, Action<T> aAction)
	{
		Context* ctx = Context::Get();
		RC_ASSERT(ctx);

		std::shared_ptr<Task<T>> task = std::make_shared<Task<T>>(aAction);

		ctx->QueueTask(aPool, aPriority, task);

		return task;
	}

	///----------------------------------------------------------------------------------------------------
	/// Run:
	/// 	Runs an action asynchronously and returns a Task object associated with it, that can be awaited.
	///----------------------------------------------------------------------------------------------------
	template <typename T>
	std::shared_ptr<Task<T>> Run(ETaskPriority aPriority, Action<T> aAction)
	{
		return Raidcore::Clockwork::Run(0, aPriority, aAction);
	}

	///----------------------------------------------------------------------------------------------------
	/// Schedule:
	/// 	Schedules a recurring action and returns a Task object associated with it.
	///----------------------------------------------------------------------------------------------------
	static std::shared_ptr<ScheduledTask> Schedule(
		uint32_t                              aPool,
		std::chrono::milliseconds             aInterval,
		Action<void>                          aAction,
		std::chrono::steady_clock::time_point aFirstExecution = std::chrono::steady_clock::now()
	)
	{
		Context* ctx = Context::Get();
		RC_ASSERT(ctx);

		std::shared_ptr<ScheduledTask> task = std::make_shared<ScheduledTask>(aInterval, aAction, aFirstExecution);

		ctx->Schedule(aPool, task);

		return task;
	}

	///----------------------------------------------------------------------------------------------------
	/// Schedule:
	/// 	Schedules a recurring action and returns a Task object associated with it.
	///----------------------------------------------------------------------------------------------------
	static std::shared_ptr<ScheduledTask> Schedule(
		std::chrono::milliseconds             aInterval,
		Action<void>                          aAction,
		std::chrono::steady_clock::time_point aFirstExecution = std::chrono::steady_clock::now()
	)
	{
		return Raidcore::Clockwork::Schedule(0, aInterval, aAction, aFirstExecution);
	}
}
