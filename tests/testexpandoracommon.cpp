#include <QtTest/QtTest>

#include "property.h"
#include "testexpandoracommon.h"

TestExpandoraCommon::TestExpandoraCommon()
    : QObject()
{}

TestExpandoraCommon::~TestExpandoraCommon() {}

void TestExpandoraCommon::skippablePropertyTest()
{
    SkipProperty property;
    QVERIFY(property.isSkipped());
    QVERIFY(property.current() == 0);
    QVERIFY_EXCEPTION_THROWN(property.rest(), std::runtime_error);

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
    QVERIFY(!property.isSkipped());
    QVERIFY2(QString(property.rest()).isEmpty(), "Expected empty string");

    // Test changing position
    QVERIFY(property.current() == '\0'); // End
    QVERIFY(property.getPos() == 11);
    QVERIFY(property.prev() == 'd'); // Rewind
    QCOMPARE(property.rest(), "d");
    QVERIFY(property.getPos() == 10);
    QVERIFY(property.next() == '\0');
    QCOMPARE(property.rest(), "");
    QVERIFY(property.getPos() == 11);
    QVERIFY(property.next() == 'h'); // Wraps around to beginning
    QVERIFY(property.getPos() == 0);
    QCOMPARE(property.rest(), "hello world");
    QVERIFY(property.next() == 'e'); // Forward
    QVERIFY(property.getPos() == 1);
    QCOMPARE(property.rest(), "ello world");
}

QTEST_MAIN(TestExpandoraCommon)
