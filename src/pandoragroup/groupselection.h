#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <cassert>
#include <cstdint>
#include <vector>

#include "../global/RuleOf5.h"
#include "CGroupChar.h"

class CGroupChar;
class GroupRecipient;

class NODISCARD GroupAdmin
{
public:
    virtual void releaseCharacters(GroupRecipient *) = 0;
    virtual ~GroupAdmin();
};

class NODISCARD GroupRecipient
{
public:
    virtual void receiveCharacters(GroupAdmin *, GroupVector) = 0;
    virtual ~GroupRecipient();
};

class NODISCARD GroupSelection final : public GroupRecipient
{
    // NOTE: deleted members must be public.
public:
    DELETE_CTORS_AND_ASSIGN_OPS(GroupSelection);

public:
    explicit GroupSelection(GroupAdmin *admin);
    virtual ~GroupSelection() override;

    void receiveCharacters(GroupAdmin *, GroupVector) override;

public:
    NODISCARD auto at(int i) const
    {
        assert(i >= 0);
        return chars.at(static_cast<uint32_t>(i));
    }
    NODISCARD auto begin() const { return chars.begin(); }
    NODISCARD auto cbegin() const { return chars.cbegin(); }
    NODISCARD auto cend() const { return chars.cend(); }
    NODISCARD auto end() const { return chars.end(); }
    NODISCARD auto size() const { return chars.size(); }
    NODISCARD auto empty() const { return chars.empty(); }

private:
    GroupAdmin *m_admin = nullptr;
    GroupVector chars;
};
