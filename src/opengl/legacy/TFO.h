#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "../../global/logging.h"
#include "Legacy.h"
#include "ScopedBinder.h"

#include <memory>

namespace Legacy {

// TFO = Transform Feedback Object
class NODISCARD TFO final
{
private:
    static constexpr GLuint INVALID_TFO = 0;
    WeakFunctions m_weakFunctions;
    GLuint m_tfo = INVALID_TFO;
    bool is_bound = false;

public:
    TFO() = default;
    ~TFO() { reset(); }

    DELETE_CTORS_AND_ASSIGN_OPS(TFO);

public:
    void emplace(const SharedFunctions &sharedFunctions);
    void reset();
    NODISCARD GLuint get() const;

public:
    NODISCARD bool isValid() const { return m_tfo != INVALID_TFO; }
    DEPRECATED_MSG("use isValid()")
    NODISCARD explicit operator bool() const { return isValid(); }

public:
    using Unbinder = ScopedBinder<TFO>;

    void unbind_impl(Badge<Unbinder>)
    {
        assert(isValid());
        assert(is_bound);
        is_bound = false;
        if (const auto shared_funcs = m_weakFunctions.lock(); shared_funcs == nullptr) {
            MMLOG_WARNING() << "Failed to unbind transform feedback buffer " << m_tfo;
        } else {
            deref(shared_funcs).glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);
        }
    }

    NODISCARD Unbinder bind()
    {
        assert(isValid());
        assert(!is_bound);

        if (const auto shared_funcs = m_weakFunctions.lock(); shared_funcs == nullptr) {
            MMLOG_WARNING() << "Failed to bind transform feedback buffer " << m_tfo;
            return Unbinder{};
        } else {
            deref(shared_funcs).glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, m_tfo);
            is_bound = true;
            return Unbinder{*this};
        }
    }
};

using SharedTfo = std::shared_ptr<TFO>;
using WeakTfo = std::weak_ptr<TFO>;

} // namespace Legacy
