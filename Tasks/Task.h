///----------------------------------------------------------------------------------------------------
/// Copyright (c) Raidcore.GG - All rights reserved.
///
/// Name         :  Task.h
/// Description  :  Definition for the Task class.
/// Authors      :  K. Bieniek
///----------------------------------------------------------------------------------------------------

#pragma once

#include <future>
#include <type_traits>

#include "TaskBase.h"

///----------------------------------------------------------------------------------------------------
/// Raidcore::Clockwork Namespace
///----------------------------------------------------------------------------------------------------
namespace Raidcore::Clockwork
{
	///----------------------------------------------------------------------------------------------------
	/// Task Class
	///----------------------------------------------------------------------------------------------------
	template <typename T>
	class Task : public ITask
	{
		public:
		///----------------------------------------------------------------------------------------------------
		/// ctor
		///----------------------------------------------------------------------------------------------------
		Task(Action<T> aAction) : Method(aAction)
		{
			this->Future = Promise.get_future();
		}

		///----------------------------------------------------------------------------------------------------
		/// Await:
		/// 	Waits for the task to complete and then returns its result, if there is one.
		///----------------------------------------------------------------------------------------------------
		T Await()
		{
			if constexpr (std::is_void_v<T>)
			{
				this->Future.get();
			}
			else
			{
				return this->Future.get();
			}
		}

		private:
		friend class Context;

		///----------------------------------------------------------------------------------------------------
		/// Execute:
		/// 	Executes the method associated with the task.
		///----------------------------------------------------------------------------------------------------
		void Execute(CancellationToken aToken) override
		{
			try
			{
				if constexpr (std::is_void_v<T>)
				{
					this->Method(aToken);
					this->Promise.set_value();
				}
				else
				{
					this->Promise.set_value(this->Method(aToken));
				}
			}
			catch (...)
			{
				this->Promise.set_exception(std::current_exception());
			}
		}

		private:
		Action<T>       Method{};

		std::promise<T> Promise;
		std::future<T>  Future;
	};
}
