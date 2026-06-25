///----------------------------------------------------------------------------------------------------
/// Copyright (c) Raidcore.GG - All rights reserved.
///
/// Name         :  TaskBase.h
/// Description  :  Definition for the Task interface class.
/// Authors      :  K. Bieniek
///----------------------------------------------------------------------------------------------------

#pragma once

#include <atomic>
#include <functional>

#include "CancellationToken.h"

///----------------------------------------------------------------------------------------------------
/// Raidcore::Clockwork Namespace
///----------------------------------------------------------------------------------------------------
namespace Raidcore::Clockwork
{
	class Context;

	template<typename T>
	using Action = std::function<T(CancellationToken aToken)>;

	///----------------------------------------------------------------------------------------------------
	/// ITask Interface Class
	///----------------------------------------------------------------------------------------------------
	class ITask
	{
		public:
		///----------------------------------------------------------------------------------------------------
		/// Cancel:
		/// 	Cancels the task, without yielding the result.
		///----------------------------------------------------------------------------------------------------
		void Cancel();

		private:
		friend class Raidcore::Clockwork::Context;

		std::atomic<bool> IsCancelled{ false };

		///----------------------------------------------------------------------------------------------------
		/// Execute:
		/// 	Executes the stored method of the ask.
		///----------------------------------------------------------------------------------------------------
		virtual void Execute(CancellationToken aToken) = 0;
	};
}
