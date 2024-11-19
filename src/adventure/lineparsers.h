#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors
// Author: Mike Repass <mike.repass@gmail.com> (Taryn)

#include "../global/macros.h"

#include <utility>

#include <QString>

class AbstractLineParser
{
protected:
    QString m_lastSuccessVal;
    bool m_pending = false;

public:
    virtual ~AbstractLineParser(); // required for warning -W non-virtual-dtor

public:
    NODISCARD bool parse(QString line) { return virt_parse(std::move(line)); }
    NODISCARD QString getLastSuccessVal() const { return m_lastSuccessVal; }

private:
    NODISCARD virtual bool virt_parse(QString line) = 0;
};

class AccomplishedTaskParser final : public AbstractLineParser
{
private:
    NODISCARD bool virt_parse(QString line) final;
};

class AchievementParser final : public AbstractLineParser
{
private:
    NODISCARD bool virt_parse(QString line) final;
};

class DiedParser final : public AbstractLineParser
{
private:
    NODISCARD bool virt_parse(QString line) final;
};

class GainedLevelParser final : public AbstractLineParser
{
private:
    NODISCARD bool virt_parse(QString line) final;
};

class HintParser final : public AbstractLineParser
{
private:
    NODISCARD bool virt_parse(QString line) final;
};

class KillAndXPParser final : public AbstractLineParser
{
private:
    int m_linesSinceShareExp = 0;

private:
    NODISCARD bool virt_parse(QString line) final;
};
