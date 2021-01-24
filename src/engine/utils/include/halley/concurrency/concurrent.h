#pragma once
#include <array>
#include <functional>
#include <halley/text/halleystring.h>
#include "executor.h"
#include "future.h"
#include "task.h"

#define HAS_THREADS 1

namespace Halley
{
	enum class ThreadPriority {
		Low,
		Normal,
		High
	};

	namespace Concurrent
	{
		template <typename F>
		auto execute(ExecutionQueue& e, F f) -> Future<typename std::result_of<F()>::type>
		{
			using R = typename std::result_of<F()>::type;
			return TaskQueueHelper<R>::enqueueOn(e, MovableFunction<R>(std::move(f)));
		}

		template <typename F>
		auto execute(F f) -> Future<typename std::result_of<F()>::type>
		{
			return execute<F>(ExecutionQueue::getDefault(), std::move(f));
		}

		template <typename Iter>
		auto whenAll(Iter begin, Iter end) -> Future<void>
		{
			JoinFuture future(int(end - begin));
			for (Iter i = begin; i != end; ++i) {
				(*i).thenNotify(future);
			}
			return future.getFuture();
		}

		template <typename T, typename F>
		void foreach(ExecutionQueue& e, T begin, T end, F f)
		{
			const size_t n = end - begin;
			constexpr size_t maxThreads = 8;
			size_t nThreads = std::max(size_t(1), std::min(maxThreads, e.threadCount()));
			std::array<Future<void>, maxThreads> futures;

			size_t prevEnd = 0;
			for (size_t j = 0; j < nThreads; ++j) {
				size_t curStart = prevEnd;
				size_t curEnd = n * (j + 1) / nThreads;
				prevEnd = curEnd;

				futures[j] = execute([begin, f, curStart, curEnd]() {
					for (auto i = begin + curStart; i < begin + curEnd; ++i) {
						f(*i);
					}
				});
			}

			whenAll(futures.data(), futures.data() + nThreads).wait();
		}

		template <typename T, typename F>
		void foreach(T begin, T end, F f)
		{
			foreach(ExecutionQueue::getDefault(), begin, end, f);
		}
	}
}
