#ifndef TESTGLOBAL_H
#define TESTGLOBAL_H

#include <QObject>

class TestGlobal : public QObject
{
    Q_OBJECT
public:
    TestGlobal();
    ~TestGlobal();

private Q_SLOTS:
    void colorTest();
};

#endif // TESTGLOBAL_H
