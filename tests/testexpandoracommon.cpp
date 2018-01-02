#include <QtTest/QtTest>

#include "testexpandoracommon.h"
#include "property.h"

using namespace std;

TestExpandoraCommon::TestExpandoraCommon()
    : QObject()
{
}

TestExpandoraCommon::~TestExpandoraCommon()
{
}

void TestExpandoraCommon::skippablePropertyTest()
{
    SkipProperty property;
    QVERIFY(property.isSkipped());
    QVERIFY(property.current() == 0);
    QVERIFY_EXCEPTION_THROWN(property.rest(), runtime_error);

    // Test changing position
    QVERIFY(property.getPos() == UINT_MAX);
    QVERIFY(property.next() == 0);
    QVERIFY(property.getPos() == 0);
    QVERIFY(property.prev() == 0);
    QVERIFY(property.getPos() == 0);
    property.reset();
    QVERIFY(property.getPos() == 0);
}

void TestExpandoraCommon::stringPropertyTest()
{
    const QByteArray ba("hello world");
    Property property(ba);
    QVERIFY(property.isSkipped() == false);
    QVERIFY2(QString(property.rest()).isEmpty(), "Expected empty string");

    // Test changing position
    QVERIFY(property.current() == '\0');
    QVERIFY(property.getPos() == 11);
    QVERIFY(property.prev() == 'd');
    QVERIFY2(property.rest(), (ba.constData() + property.getPos()));
    QVERIFY(property.getPos() == 10);
    QVERIFY(property.next() == '\0');
    QVERIFY(property.getPos() == 11);
    QVERIFY(property.next() == 'h'); // Wraps around to beginning
    QVERIFY(property.getPos() == 0);
    QVERIFY2(property.rest(), ba);
}

QTEST_MAIN(TestExpandoraCommon)
