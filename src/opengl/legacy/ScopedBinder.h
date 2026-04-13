#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "../../global/Badge.h"
#include "../../global/RuleOf5.h"
#include "../../global/macros.h"

namespace Legacy {

// Generic move-only RAII unbind guard. Owner must provide a public method
//   void unbind_impl(Badge<ScopedBinder<Owner>>);
// which is called exactly once from this guard's destructor, unless the
// guard was default-constructed (e.g. because bind() failed to lock its
// weak_ptr to Functions).
template<typename Owner>
class NODISCARD ScopedBinder final
{
private:
    Owner *m_self = nullptr;

public:
    explicit ScopedBinder() = default;
    explicit ScopedBinder(Owner &self)
        : m_self{&self}
    {}
    DELETE_CTORS_AND_ASSIGN_OPS(ScopedBinder);
    ~ScopedBinder()
    {
        if (m_self != nullptr) {
            m_self->unbind_impl(Badge<ScopedBinder<Owner>>{});
        }
    }
};

} // namespace Legacy
