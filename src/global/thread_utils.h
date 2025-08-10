#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "macros.h"
#include "mm_source_location.h"
#include "progresscounter.h"

#include <exception>
#include <execution>
#include <future>
#include <thread>

namespace thread_utils {

NODISCARD extern bool isOnMainThread();

#define ABORT_IF_NOT_ON_MAIN_THREAD() ::thread_utils::abortIfNotOnMainThread(MM_SOURCE_LOCATION())
extern void abortIfNotOnMainThread(mm::source_location loc);

template<typename ThreadLocals, typename Container, typename Callback, typename MergeThreadLocals>
void parallel_for_each_tl_range(Container &&container,
                                ProgressCounter &counter,
                                Callback &&callback,
                                MergeThreadLocals &&merge_threadlocals)
{
#ifdef Q_OS_WASM
    const auto numThreads = 1;
#else
    const auto numThreads = std::max<size_t>(1, std::thread::hardware_concurrency());
#endif
    if (numThreads == 1) {
        std::array<ThreadLocals, 1> thread_locals;
        auto &tl = thread_locals.front();
        callback(tl, std::begin(container), std::end(container));
        merge_threadlocals(thread_locals);
    } else {
        std::vector<ThreadLocals> thread_locals;
        thread_locals.reserve(numThreads);
        std::vector<std::future<void>> futures;
        futures.reserve(numThreads);

        auto outer_it = container.begin();
        const size_t chunkSize = (container.size() + numThreads - 1) / numThreads;
        for (size_t i = 0; i < numThreads && outer_it != container.end(); ++i) {
            const auto chunkBegin = outer_it;
            const auto remaining = static_cast<size_t>(std::distance(outer_it, container.end()));
            const size_t actualChunkSize = std::min(chunkSize, remaining);
            std::advance(outer_it, actualChunkSize);
            const auto chunkEnd = outer_it;

            auto &tl = thread_locals.emplace_back();
            futures.emplace_back(
                std::async(std::launch::async, [&counter, &callback, chunkBegin, chunkEnd, &tl]() {
                    try {
                        callback(tl, chunkBegin, chunkEnd);
                    } catch (...) {
                        counter.requestCancel();
                        std::rethrow_exception(std::current_exception());
                    }
                }));
        }

        // these are checked sequentially, so requesting cancel won't help.
        bool canceled = false;
        std::exception_ptr exception;
        for (auto &fut : futures) {
            try {
                fut.get();
            } catch (const ProgressCanceledException &) {
                canceled = true;
            } catch (...) {
                if (!exception) {
                    exception = std::current_exception();
                }
            }
        }
        if (exception) {
            std::rethrow_exception(exception);
        } else if (canceled) {
            throw ProgressCanceledException();
        }

        merge_threadlocals(thread_locals);
    }
}

template<typename ThreadLocals, typename Container, typename Callback, typename MergeThreadLocals>
void parallel_for_each_tl(Container &&container,
                          ProgressCounter &counter,
                          Callback &&callback,
                          MergeThreadLocals &&merge_threadlocals)
{
    parallel_for_each_tl_range<ThreadLocals>(
        std::forward<Container>(container),
        counter,
        [&counter, &callback](auto &tl, const auto chunkBegin, const auto chunkEnd) {
            for (auto it = chunkBegin; it != chunkEnd; ++it) {
                callback(tl, *it);
                counter.step();
            }
        },
        std::forward<MergeThreadLocals>(merge_threadlocals));
}

template<typename Container, typename Callback>
void parallel_for_each(Container &&container, ProgressCounter &counter, Callback &&callback)
{
    struct DummyThreadLocals final
    {};
    parallel_for_each_tl<DummyThreadLocals>(
        std::forward<Container>(container),
        counter,
        [&callback](DummyThreadLocals &, auto &&x) { callback(x); },
        // merge thread locals
        [](auto &) {});
}

} // namespace thread_utils
