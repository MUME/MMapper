#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/RuleOf5.h"
#include "CGroupChar.h"

#include <cassert>
#include <cstdint>
#include <vector>

class CGroupChar;
class GroupRecipient;

class NODISCARD GroupAdmin
{
public:
    virtual ~GroupAdmin();

private:
    virtual void virt_releaseCharacters(GroupRecipient *) = 0;

public:
    void releaseCharacters(GroupRecipient *const groupRecipient)
    {
        virt_releaseCharacters(groupRecipient);
    }
};

class NODISCARD GroupRecipient
{
public:
    virtual ~GroupRecipient();

private:
    virtual void virt_receiveCharacters(GroupAdmin *, GroupVector v) = 0;

public:
    void receiveCharacters(GroupAdmin *const admin, GroupVector v)
    {
        virt_receiveCharacters(admin, std::move(v));
    }
};

class NODISCARD GroupSelection final : public GroupRecipient
{
private:
    GroupAdmin *m_admin = nullptr;
    GroupVector m_chars;

public:
    DELETE_CTORS_AND_ASSIGN_OPS(GroupSelection);

public:
    explicit GroupSelection(GroupAdmin *admin);
    ~GroupSelection() final;

private:
    void virt_receiveCharacters(GroupAdmin *admin, GroupVector chars) final;

public:
    NODISCARD auto at(const int i) const
    {
        assert(i >= 0);
        return m_chars.at(static_cast<uint32_t>(i));
    }
    NODISCARD auto begin() const { return m_chars.begin(); }
    NODISCARD auto cbegin() const { return m_chars.cbegin(); }
    NODISCARD auto cend() const { return m_chars.cend(); }
    NODISCARD auto end() const { return m_chars.end(); }
    NODISCARD auto size() const { return m_chars.size(); }
    NODISCARD auto empty() const { return m_chars.empty(); }
};
