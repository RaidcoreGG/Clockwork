///----------------------------------------------------------------------------------------------------
/// Copyright (c) Raidcore.GG - All rights reserved.
///
/// Name         :  TaskBase.cpp
/// Description  :  Definition for the Task interface class.
/// Authors      :  K. Bieniek
///----------------------------------------------------------------------------------------------------

#include "TaskBase.h"

namespace Raidcore::Clockwork
{
	void ITask::Cancel()
	{
		this->IsCancelled.store(true);
	}
}
