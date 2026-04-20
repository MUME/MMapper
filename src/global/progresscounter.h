#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Thomas Equeter <waba@waba.be> (Waba)

#include "Badge.h"
#include "TaggedString.h"
#include "macros.h"

#include <cstdint>
#include <mutex>

namespace tags {
struct NODISCARD TagProgressMsg final
{
    NODISCARD static bool isValid(std::string_view sv);
};
} // namespace tags

using ProgressMsg = TaggedBoxedStringUtf8<tags::TagProgressMsg>;
enum class NODISCARD AllowCancelEnum : uint8_t { Forbid, Allow };

struct NODISCARD ProgressCanceledException final : public std::runtime_error
{
    ProgressCanceledException();
    ~ProgressCanceledException() final;
};

class NODISCARD ProgressCounter final
{
public:
    struct NODISCARD Status final
    {
        ProgressMsg msg;
        size_t expected = 0;
        size_t seen = 0;
        NODISCARD size_t percent() const
        {
            if (expected == 0) {
                return 0;
            }
            return std::clamp<size_t>((100 * seen) / expected, 0, 100);
        }
        void reset(Badge<ProgressCounter>, size_t expected_ = 0)
        {
            seen = 0;
            expected = expected_;
        }
    };

private:
    // note: Having a mutable member variable effectively means the object is never actually const,
    // but it allows us to keep the distinction between read-only and read-write member functions.
    mutable std::mutex m_mutex;
    Status m_status;
    const AllowCancelEnum m_allowCancel = AllowCancelEnum::Allow;
    std::atomic_bool m_requested_cancel{false};

public:
    explicit ProgressCounter(const AllowCancelEnum allowCancel = AllowCancelEnum::Allow)
        : m_allowCancel{allowCancel}
    {}
    ~ProgressCounter() = default;
    ProgressCounter(const ProgressCounter &) = delete;
    ProgressCounter &operator=(const ProgressCounter &) = delete;

private:
    void checkCancel() const
    {
        if (hasRequestedCancel()) {
            throw ProgressCanceledException();
        }
    }

public:
    void setNewTask(const ProgressMsg &currentTask, size_t newTotalSteps);
    void setCurrentTask(const ProgressMsg &currentTask);
    void step(size_t steps = 1u);
    void increaseTotalStepsBy(size_t steps);
    void reset();
    void requestCancel()
    {
        if (allowCancel() == AllowCancelEnum::Allow) {
            m_requested_cancel = true;
        }
    }

public:
    NODISCARD ProgressMsg getCurrentTask() const;
    NODISCARD size_t getPercentage() const;
    NODISCARD Status getStatus() const;
    NODISCARD AllowCancelEnum allowCancel() const { return m_allowCancel; }
    NODISCARD bool hasRequestedCancel() const { return m_requested_cancel; }
};
