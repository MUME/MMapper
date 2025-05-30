#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/RuleOf5.h"
#include "../global/StringView.h"

#include <functional>
#include <memory>
#include <regex>
#include <string>
#include <unordered_map>
#include <utility>

using ActionCallback = std::function<void(StringView)>;

struct NODISCARD IAction
{
public:
    virtual ~IAction();
    IAction() = default;
    DEFAULT_CTORS_AND_ASSIGN_OPS(IAction);

private:
    virtual void virt_match(StringView input) const = 0;

public:
    void match(StringView input) const;
};

class NODISCARD StartsWithAction : public IAction
{
private:
    const std::string m_match;
    const ActionCallback m_callback;

public:
    explicit StartsWithAction(std::string moved_str, const ActionCallback &callback)
        : m_match{std::move(moved_str)}
        , m_callback{callback}
    {}

private:
    void virt_match(StringView input) const override;
};

class NODISCARD EndsWithAction : public IAction
{
private:
    const std::string m_match;
    const ActionCallback m_callback;

public:
    explicit EndsWithAction(std::string moved_str, const ActionCallback &callback)
        : m_match{std::move(moved_str)}
        , m_callback{callback}
    {}

private:
    void virt_match(StringView input) const override;
};

class NODISCARD RegexAction : public IAction
{
private:
    const std::regex m_regex;
    const ActionCallback m_callback;

public:
    explicit RegexAction(const std::string &pattern, const ActionCallback &callback);

private:
    void virt_match(StringView input) const override;
};

using ActionHint = char;
using ActionRecordMap = std::unordered_multimap<ActionHint, std::unique_ptr<IAction>>;
