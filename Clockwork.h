///----------------------------------------------------------------------------------------------------
/// Copyright (c) Raidcore.GG - All rights reserved.
///
/// Name         :  Clockwork.h
/// Description  :  Definition for Task class.
/// Authors      :  K. Bieniek
///----------------------------------------------------------------------------------------------------

#pragma once

#include <assert.h>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

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
		static Context* Create(uint32_t aThreadPoolCount = 1, uint32_t aThreadPoolSize = 0);

		///----------------------------------------------------------------------------------------------------
		/// Get:
		/// 	Returns the Clockwork context.
		///----------------------------------------------------------------------------------------------------
		static Context* Get();

		///----------------------------------------------------------------------------------------------------
		/// Destroy:
		/// 	Destroys the Clockwork context.
		///----------------------------------------------------------------------------------------------------
		static void Destroy();

		///----------------------------------------------------------------------------------------------------
		/// Init:
		/// 	Initializes the threadpools.
		///----------------------------------------------------------------------------------------------------
		void Init();

		///----------------------------------------------------------------------------------------------------
		/// QueueTask:
		/// 	Queues a task to the threadpool, waiting for completion.
		///----------------------------------------------------------------------------------------------------
		void QueueTask(uint32_t aPool, ETaskPriority aPriority, std::shared_ptr<ITask> aTask);

		///----------------------------------------------------------------------------------------------------
		/// Schedule:
		/// 	Schedules a recurring action.
		///----------------------------------------------------------------------------------------------------
		void Schedule(uint32_t aPool, std::shared_ptr<ScheduledTask> aTask);
		
		Context(Context const&) = delete;
		void operator=(Context const&) = delete;

		private:
		bool                     IsRunning{ false };
		uint32_t                 ThreadPoolCount{ 1 };
		uint32_t                 ThreadPoolSize{ 1 };
		std::vector<Threadpool*> ThreadPools{};

		///----------------------------------------------------------------------------------------------------
		/// ctor
		///----------------------------------------------------------------------------------------------------
		Context(uint32_t aThreadPoolCount, uint32_t aThreadPoolSize);

		///----------------------------------------------------------------------------------------------------
		/// dtor
		///----------------------------------------------------------------------------------------------------
		~Context();

		///----------------------------------------------------------------------------------------------------
		/// WorkerSetup:
		/// 	Setup function for the worker threads.
		///----------------------------------------------------------------------------------------------------
		void WorkerSetup(uint32_t aPool, uint32_t aThread);

		///----------------------------------------------------------------------------------------------------
		/// WorkerLoop:
		/// 	Loop function for the worker threads.
		///----------------------------------------------------------------------------------------------------
		void WorkerLoop(Threadpool* aPool);

		///----------------------------------------------------------------------------------------------------
		/// SchedulerSetup:
		/// 	Setup function for the schedculer threads.
		///----------------------------------------------------------------------------------------------------
		void SchedulerSetup(uint32_t aPool);

		///----------------------------------------------------------------------------------------------------
		///	SchedulerLoop:
		/// 	Loop function for the scheduler threads.
		///----------------------------------------------------------------------------------------------------
		void SchedulerLoop(Threadpool* aPool);
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
