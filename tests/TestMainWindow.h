#ifndef TESTMAINWINDOW_H
#define TESTMAINWINDOW_H

#include <QObject>

class TestMainWindow : public QObject
{
    Q_OBJECT
public:
    TestMainWindow();
    ~TestMainWindow();

private:
private Q_SLOTS:
    void updaterTest();
};

#endif // TESTMAINWINDOW_H
