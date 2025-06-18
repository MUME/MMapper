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

template<typename Container, typename Callback>
void parallel_for_each(Container &&container, ProgressCounter &counter, Callback &&callback)
{
    const auto numThreads = std::max<size_t>(1, std::thread::hardware_concurrency());

    if (numThreads == 1) {
        for (auto &&id : container) {
            callback(id);
            counter.step();
        }
    } else {
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

            futures.emplace_back(
                std::async(std::launch::async, [&counter, &callback, chunkBegin, chunkEnd]() {
                    try {
                        for (auto iter = chunkBegin; iter != chunkEnd; ++iter) {
                            callback(*iter);
                            counter.step();
                        }
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
    }
}

} // namespace thread_utils
