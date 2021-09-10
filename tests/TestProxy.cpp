// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "TestProxy.h"

#include <QDebug>
#include <QtTest/QtTest>

#include "../src/global/TextUtils.h"
#include "../src/proxy/GmcpMessage.h"
#include "../src/proxy/GmcpModule.h"
#include "../src/proxy/GmcpUtils.h"

void TestProxy::escapeTest()
{
    QCOMPARE(GmcpUtils::escapeGmcpStringData(R"(12345)"), QString(R"(12345)"));
    QCOMPARE(GmcpUtils::escapeGmcpStringData(R"(1.0)"), QString(R"(1.0)"));
    QCOMPARE(GmcpUtils::escapeGmcpStringData(R"(true)"), QString(R"(true)"));
    QCOMPARE(GmcpUtils::escapeGmcpStringData(R"("Hello")"), QString(R"(\"Hello\")"));
    QCOMPARE(GmcpUtils::escapeGmcpStringData("\\\n\r\b\f\t"), QString(R"(\\\n\r\b\f\t)"));
}

void TestProxy::gmcpMessageDeserializeTest()
{
    GmcpMessage gmcp1 = GmcpMessage::fromRawBytes(R"(Core.Hello { "Hello": "world" })");
    QCOMPARE(gmcp1.getName().toQByteArray(), QByteArray("Core.Hello"));
    QCOMPARE(gmcp1.getJson()->toQString(), ::toQStringUtf8(R"({ "Hello": "world" })"));

    GmcpMessage gmcp2 = GmcpMessage::fromRawBytes(R"(Core.Goodbye)");
    QCOMPARE(gmcp2.getName().toQByteArray(), QByteArray("Core.Goodbye"));
    QVERIFY(!gmcp2.getJson());

    GmcpMessage gmcp3 = GmcpMessage::fromRawBytes(R"(External.Discord.Hello)");
    QCOMPARE(gmcp3.getName().toQByteArray(), QByteArray("External.Discord.Hello"));
    QVERIFY(!gmcp3.getJson());
}

void TestProxy::gmcpMessageSerializeTest()
{
    GmcpMessage gmcp1(GmcpMessageTypeEnum::CORE_HELLO);
    QCOMPARE(gmcp1.toRawBytes(), QByteArray("Core.Hello"));

    GmcpMessage gmcp2(GmcpMessageTypeEnum::CORE_HELLO, QString("{}"));
    QCOMPARE(gmcp2.toRawBytes(), QByteArray("Core.Hello {}"));
}

void TestProxy::gmcpModuleTest()
{
    GmcpModule module1(QString("Char 1"));
    QCOMPARE(::toQByteArrayLatin1(module1.getNormalizedName()), QByteArray("char"));
    QCOMPARE(module1.getVersion().asUint32(), 1u);
    QVERIFY(module1.isSupported());

    GmcpModule module2(QString("Char.Skills 1"));
    QCOMPARE(::toQByteArrayLatin1(module2.getNormalizedName()), QByteArray("char.skills"));
    QCOMPARE(module2.getVersion().asUint32(), 1u);
    QVERIFY(!module2.isSupported());

    GmcpModule module3(QString("Room"));
    QCOMPARE(::toQByteArrayLatin1(module3.getNormalizedName()), QByteArray("room"));
    QCOMPARE(module3.getVersion().asUint32(), 0u);
    QVERIFY(!module3.isSupported());

    GmcpModule module4(QString("MMapper.Comm 1"));
    QCOMPARE(::toQByteArrayLatin1(module4.getNormalizedName()), QByteArray("mmapper.comm"));
    QCOMPARE(module4.getVersion().asUint32(), 1u);
    QVERIFY(module4.isSupported());
}

QTEST_MAIN(TestProxy)
