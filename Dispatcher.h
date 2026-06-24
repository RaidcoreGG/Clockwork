///----------------------------------------------------------------------------------------------------
/// Copyright (c) Raidcore.GG - All rights reserved.
///
/// Name         :  Dispatcher.h
/// Description  :  Definition for the Dispatcher class.
/// Authors      :  K. Bieniek
///----------------------------------------------------------------------------------------------------

#pragma once

#include "Clockwork.h"
#include "Tasks/ETaskPriority.h"
#include "Tasks/TaskBase.h"

///----------------------------------------------------------------------------------------------------
/// Raidcore::Clockwork Namespace
///----------------------------------------------------------------------------------------------------
namespace Raidcore::Clockwork
{
	///----------------------------------------------------------------------------------------------------
	/// Dispatcher Class
	///----------------------------------------------------------------------------------------------------
	template <typename T>
	class Dispatcher
	{
		public:
		///----------------------------------------------------------------------------------------------------
		/// ctor
		///----------------------------------------------------------------------------------------------------
		Dispatcher()
			: Method()
			, Pool(0)
			, Priority(ETaskPriority::Low)
		{}

		///----------------------------------------------------------------------------------------------------
		/// ctor
		///----------------------------------------------------------------------------------------------------
		Dispatcher(Action<T> aAction, uint32_t aPool = 0, ETaskPriority aPriority = ETaskPriority::Low)
			: Method(std::move(aAction))
			, Pool(aPool)
			, Priority(aPriority)
		{}

		///----------------------------------------------------------------------------------------------------
		/// SetAction:
		/// 	Sets the action for the dispatcher.
		///----------------------------------------------------------------------------------------------------
		void SetAction(Action<T> aAction)
		{
			this->Method = std::move(aAction);
		}

		///----------------------------------------------------------------------------------------------------
		/// SetPool:
		/// 	Sets the pool for the dispatcher.
		///----------------------------------------------------------------------------------------------------
		void SetPool(uint32_t aPool)
		{
			this->Pool = aPool;
		}

		///----------------------------------------------------------------------------------------------------
		/// SetPriority:
		/// 	Sets the priority for the dispatcher.
		///----------------------------------------------------------------------------------------------------
		void SetPriority(ETaskPriority aPriority)
		{
			this->Priority = aPriority;
		}

		///----------------------------------------------------------------------------------------------------
		/// Dispatch:
		/// 	Dispatches a worker task.
		///----------------------------------------------------------------------------------------------------
		std::shared_ptr<Task<T>> Dispatch()
		{
			RC_ASSERT(this->Method && "Dispatcher Method is not set.");

			return Raidcore::Clockwork::Run<T>(
				this->Pool,
				this->Priority,
				this->Method
			);
		}

		///----------------------------------------------------------------------------------------------------
		/// operator():
		/// 	Dispatches a worker task.
		///----------------------------------------------------------------------------------------------------
		std::shared_ptr<Task<T>> operator()() const
		{
			RC_ASSERT(this->Method && "Dispatcher Method is not set.");

			return Raidcore::Clockwork::Run<T>(
				this->Pool,
				this->Priority,
				this->Method
			);
		}

		private:
		Action<T>     Method  {};
		uint32_t      Pool    { 0 };
		ETaskPriority Priority{ ETaskPriority::Low };
	};
}
