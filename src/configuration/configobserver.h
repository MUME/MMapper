#pragma once

#include <QObject>

// Added this one-off helper file since Qt MOC couldn't handle all the MACROs in Configuration.h
class ConfigObserver final : public QObject
{
    Q_OBJECT

public:
    static ConfigObserver &get()
    {
        static ConfigObserver singleton;
        return singleton;
    }

signals:
    void sig_configChanged();

private:
    ConfigObserver() {}
};
