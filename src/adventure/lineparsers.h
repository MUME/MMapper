#pragma once

#include <QString>

class AbstractStatefulLineParser
{
public:
    virtual ~AbstractStatefulLineParser();
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
