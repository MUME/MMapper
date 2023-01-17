#pragma once

#include <QString>

class AbstractStatefulLineParser
{
public:
    virtual bool parse(QString line) = 0;
    QString getLastSuccessVal() const { return m_lastSuccessVal; }

protected:
    bool m_pending = false;
    QString m_lastSuccessVal;
};

class AchievementParser : public AbstractStatefulLineParser
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
