///----------------------------------------------------------------------------------------------------
/// Copyright (c) Raidcore.GG - All rights reserved.
///
/// Name         :  WorkerTask.h
/// Description  :  Definition for the WorkerTask class.
/// Authors      :  K. Bieniek
///----------------------------------------------------------------------------------------------------

#pragma once

#include "ETaskPriority.h"
#include "TaskBase.h"

///----------------------------------------------------------------------------------------------------
/// Raidcore::Clockwork Namespace
///----------------------------------------------------------------------------------------------------
namespace Raidcore::Clockwork
{
	///----------------------------------------------------------------------------------------------------
	/// WorkerTask Class
	///----------------------------------------------------------------------------------------------------
	class WorkerTask : public virtual ITask
	{
		public:
		///----------------------------------------------------------------------------------------------------
		/// ctor
		///----------------------------------------------------------------------------------------------------
		WorkerTask(WorkAction aAction, uint32_t aPool = 0, ETaskPriority aPriority = ETaskPriority::Low)
			: Method(aAction)
			, Pool(aPool)
			, Priority(aPriority)
		{}

		///----------------------------------------------------------------------------------------------------
		/// Dispatch:
		/// 	Dispatches a worker task.
		///----------------------------------------------------------------------------------------------------
		void Dispatch();

		private:
		friend class Context;

		WorkAction    Method  {};
		uint32_t      Pool    { 0 };
		ETaskPriority Priority{ ETaskPriority::Low };

		///----------------------------------------------------------------------------------------------------
		/// Execute:
		/// 	Executes the method associated with the task.
		///----------------------------------------------------------------------------------------------------
		void Execute(CancellationToken aToken) override;
	};
}
