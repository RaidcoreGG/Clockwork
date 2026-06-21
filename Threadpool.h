///----------------------------------------------------------------------------------------------------
/// Copyright (c) Raidcore.GG - All rights reserved.
///
/// Name         :  Threadpool.h
/// Description  :  Definition for the Threadpool.
/// Authors      :  K. Bieniek
///----------------------------------------------------------------------------------------------------

#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include "Tasks/ETaskPriority.h"
#include "Tasks/TaskBase.h"

///----------------------------------------------------------------------------------------------------
/// Raidcore::Clockwork Namespace
///----------------------------------------------------------------------------------------------------
namespace Raidcore::Clockwork
{
	///----------------------------------------------------------------------------------------------------
	/// Threadpool Struct
	///----------------------------------------------------------------------------------------------------
	struct Threadpool
	{
		std::vector<std::thread>              Threads{};

		std::mutex                            TaskMutex{};
		std::condition_variable               TaskConVar{};
		std::queue<std::shared_ptr<ITask>>    TaskQueue[static_cast<uint32_t>(ETaskPriority::COUNT)]{};
	};
}
