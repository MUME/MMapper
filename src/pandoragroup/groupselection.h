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

class GroupAdmin
{
public:
    virtual void releaseCharacters(GroupRecipient *) = 0;
    virtual ~GroupAdmin();
};

class GroupRecipient
{
public:
    virtual void receiveCharacters(GroupAdmin *, GroupVector) = 0;
    virtual ~GroupRecipient();
};

class GroupSelection final : public GroupRecipient
{
    // NOTE: deleted members must be public.
public:
    DELETE_CTORS_AND_ASSIGN_OPS(GroupSelection);

public:
    explicit GroupSelection(GroupAdmin *admin);
    virtual ~GroupSelection() override;

    void receiveCharacters(GroupAdmin *, GroupVector) override;

public:
    auto at(int i) const
    {
        assert(i >= 0);
        return chars.at(static_cast<uint32_t>(i));
    }
    auto begin() const { return chars.begin(); }
    auto cbegin() const { return chars.cbegin(); }
    auto cend() const { return chars.cend(); }
    auto end() const { return chars.end(); }
    auto size() const { return chars.size(); }
    auto empty() const { return chars.empty(); }

private:
    GroupAdmin *m_admin = nullptr;
    GroupVector chars;
};
