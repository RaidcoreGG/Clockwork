///----------------------------------------------------------------------------------------------------
/// Copyright (c) Raidcore.GG - All rights reserved.
///
/// Name         :  Task.h
/// Description  :  Definition for the Task class.
/// Authors      :  K. Bieniek
///----------------------------------------------------------------------------------------------------

#pragma once

#include <future>
#include <optional>
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
	class Task : public virtual ITask
	{
		public:
		///----------------------------------------------------------------------------------------------------
		/// ctor
		///----------------------------------------------------------------------------------------------------
		Task(Action<T> aAction) : Method(aAction)
		{
			this->Future = this->Promise.get_future().share();
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

		///----------------------------------------------------------------------------------------------------
		/// Completed:
		/// 	Returns true if the task has completed, false otherwise.
		///----------------------------------------------------------------------------------------------------
		bool Completed() const
		{
			return this->Future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready;
		}

		///----------------------------------------------------------------------------------------------------
		/// Result:
		/// 	Returns the result of the task, if there is one.
		/// 	If the task has not completed yet, this will block until it does.
		///----------------------------------------------------------------------------------------------------
		T Result()
		{
			if constexpr (std::is_void_v<T>)
			{
				this->Await();
			}
			else
			{
				return this->Await();
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
		Action<T>             Method{};

		std::promise<T>       Promise;
		std::shared_future<T> Future;
	};
}
