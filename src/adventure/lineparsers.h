#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors
// Author: Mike Repass <mike.repass@gmail.com> (Taryn)

#include <QString>

class AbstractStatefulLineParser
{
public:
    virtual ~AbstractStatefulLineParser(); // required for warning -W non-virtual-dtor
    virtual bool parse(QString line) = 0;
    virtual QString getLastSuccessVal();

protected:
    bool m_pending = false;
    QString m_lastSuccessVal;
};

class AccomplishedTaskParser : public AbstractStatefulLineParser
{
public:
    bool parse(QString line) override;
};

class AchievementParser : public AbstractStatefulLineParser
{
public:
    bool parse(QString line) override;
};

class DiedParser : public AbstractStatefulLineParser
{
public:
    bool parse(QString line) override;
};

class GainedLevelParser : public AbstractStatefulLineParser
{
public:
    bool parse(QString line) override;
};

class HintParser : public AbstractStatefulLineParser
{
public:
    bool parse(QString line) override;
};

class KillAndXPParser : public AbstractStatefulLineParser
{
public:
    bool parse(QString line) override;

private:
    int m_linesSinceShareExp = 0;
};
