///----------------------------------------------------------------------------------------------------
/// Copyright (c) Raidcore.GG - All rights reserved.
///
/// Name         :  Clockwork.h
/// Description  :  Definition for Task class.
/// Authors      :  K. Bieniek
///----------------------------------------------------------------------------------------------------

#pragma once

#include <assert.h>
#include <condition_variable>
#include <mutex>
#include <queue>

#include "Tasks/ETaskPriority.h"
#include "Tasks/ScheduledTask.h"
#include "Tasks/Task.h"
#include "Tasks/TaskBase.h"
#include "Tasks/WorkerTask.h"

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
		void QueueTask(ETaskPriority aPriority, std::shared_ptr<ITask> aTask);

		Context(Context const&) = delete;
		void operator=(Context const&) = delete;

		private:
		bool                                  IsRunning{ false };
		uint32_t                              ThreadPoolCount{ 1 };
		uint32_t                              ThreadPoolSize{ 1 };
		std::vector<std::vector<std::thread>> ThreadPools{};

		std::mutex                            TaskMutex{};
		std::condition_variable               TaskConVar{};
		std::queue<std::shared_ptr<ITask>>    TaskQueue[static_cast<uint32_t>(ETaskPriority::COUNT)]{};

		///----------------------------------------------------------------------------------------------------
		/// ctor
		///----------------------------------------------------------------------------------------------------
		Context(uint32_t aThreadPoolCount, uint32_t aThreadPoolSize);

		///----------------------------------------------------------------------------------------------------
		/// dtor
		///----------------------------------------------------------------------------------------------------
		~Context();

		///----------------------------------------------------------------------------------------------------
		/// WorkerLoop:
		/// 	Loop function for the worker threads.
		///----------------------------------------------------------------------------------------------------
		void WorkerLoop();
	};

	///----------------------------------------------------------------------------------------------------
	/// Run:
	/// 	Runs an action asynchronously and returns a Task object associated with it, that can be awaited.
	///----------------------------------------------------------------------------------------------------
	template <typename T>
	std::shared_ptr<Task<T>> Run(ETaskPriority aPriority, Action<T> aAction)
	{
		Context* ctx = Context::Get();
		RC_ASSERT(ctx);

		std::shared_ptr<Task<T>> task = std::make_shared<Task<T>>(aAction);

		ctx->QueueTask(aPriority, task);

		return task;
	}

	///----------------------------------------------------------------------------------------------------
	/// Schedule:
	/// 	Schedules a recurring action and returns a Task object associated with it.
	///----------------------------------------------------------------------------------------------------
	/*static std::shared_ptr<ScheduledTask> Schedule(uint64_t aIntervalMs, WorkAction aAction)
	{
		Context* ctx = Context::Get();
		RC_ASSERT(ctx);
	}*/

	///----------------------------------------------------------------------------------------------------
	/// CreateWorker:
	/// 	Creates a worker action, that runs when work is queued.
	///----------------------------------------------------------------------------------------------------
	/*static std::shared_ptr<WorkerTask> CreateWorker(uint64_t aIntervalMs, WorkAction aAction)
	{
		Context* ctx = Context::Get();
		RC_ASSERT(ctx);
	}*/
}
