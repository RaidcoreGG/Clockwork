///----------------------------------------------------------------------------------------------------
/// Copyright (c) Raidcore.GG - All rights reserved.
///
/// Name         :  WorkerTask.cpp
/// Description  :  Definition for the WorkerTask class.
/// Authors      :  K. Bieniek
///----------------------------------------------------------------------------------------------------

#include "WorkerTask.h"

#include "../Clockwork.h"

namespace Raidcore::Clockwork
{
	void WorkerTask::Dispatch()
	{
		Raidcore::Clockwork::Run(this->Pool, this->Priority, this->Method);
	}

	void WorkerTask::Execute(CancellationToken aToken)
	{
		try
		{
			this->Method(aToken);
		}
		catch (...)
		{
			/* NOP */
		}
	}
}
