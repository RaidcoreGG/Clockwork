///----------------------------------------------------------------------------------------------------
/// Copyright (c) Raidcore.GG - All rights reserved.
///
/// Name         :  ScheduledTask.h
/// Description  :  Definition for the ScheduledTask class.
/// Authors      :  K. Bieniek
///----------------------------------------------------------------------------------------------------

#pragma once

#include <chrono>

#include "TaskBase.h"

///----------------------------------------------------------------------------------------------------
/// Raidcore::Clockwork Namespace
///----------------------------------------------------------------------------------------------------
namespace Raidcore::Clockwork
{
	///----------------------------------------------------------------------------------------------------
	/// ScheduledTask Class
	///----------------------------------------------------------------------------------------------------
	class ScheduledTask : public virtual ITask
	{
		public:
		///----------------------------------------------------------------------------------------------------
		/// ctor
		///----------------------------------------------------------------------------------------------------
		ScheduledTask(
			std::chrono::milliseconds aInterval,
			Action<void> aAction,
			std::chrono::steady_clock::time_point aFirstExecution = std::chrono::steady_clock::now()
		)
			: NextExecution(aFirstExecution)
			, Interval(aInterval)
			, Method(std::move(aAction))
		{}

		///----------------------------------------------------------------------------------------------------
		/// GetNextExecution:
		/// 	Gets the time point of the next execution of the scheduled task.
		///----------------------------------------------------------------------------------------------------
		std::chrono::steady_clock::time_point GetNextExecution() const
		{
			return this->NextExecution;
		}

		private:
		friend class Raidcore::Clockwork::Context;

		std::chrono::steady_clock::time_point NextExecution{};
		std::chrono::milliseconds             Interval{ 0 };
		Action<void>                          Method{};

		///----------------------------------------------------------------------------------------------------
		/// Reschedule:
		/// 	Reschedules the execution of the task by adding the interval to the next execution time.
		///----------------------------------------------------------------------------------------------------
		void Reschedule()
		{
			this->NextExecution += this->Interval;
		}

		///----------------------------------------------------------------------------------------------------
		/// Execute:
		/// 	Executes the method associated with the task.
		///----------------------------------------------------------------------------------------------------
		void Execute(CancellationToken aToken) override
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
	};
}
