// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "TestGlobal.h"

#include "../src/global/AnsiOstream.h"
#include "../src/global/AnsiTextUtils.h"
#include "../src/global/CaseUtils.h"
#include "../src/global/CharUtils.h"
#include "../src/global/Diff.h"
#include "../src/global/Flags.h"
#include "../src/global/HideQDebug.h"
#include "../src/global/IndexedVectorWithDefault.h"
#include "../src/global/LineUtils.h"
#include "../src/global/RAII.h"
#include "../src/global/Signal2.h"
#include "../src/global/StringView.h"
#include "../src/global/TaggedString.h"
#include "../src/global/TextUtils.h"
#include "../src/global/WeakHandle.h"
#include "../src/global/emojis.h"
#include "../src/global/entities.h"
#include "../src/global/float_cast.h"
#include "../src/global/int_cast.h"
#include "../src/global/string_view_utils.h"
#include "../src/global/unquote.h"

#include <tuple>

#include <QDebug>
#include <QtTest/QtTest>

TestGlobal::TestGlobal() = default;

TestGlobal::~TestGlobal() = default;

void TestGlobal::ansi256ColorTest()
{
    const int blackAnsi = rgbToAnsi256(0, 0, 0);
    QCOMPARE(blackAnsi, 16);
    const QColor blackRgb = mmqt::ansi256toRgb(blackAnsi);
    QCOMPARE(blackRgb, QColor(Qt::black));

    const int redAnsi = rgbToAnsi256(255, 0, 0);
    QCOMPARE(redAnsi, 196);
    const QColor redRgb = mmqt::ansi256toRgb(redAnsi);
    QCOMPARE(redRgb, QColor(Qt::red));

    const int greenAnsi = rgbToAnsi256(0, 255, 0);
    QCOMPARE(greenAnsi, 46);
    const QColor greenRgb = mmqt::ansi256toRgb(greenAnsi);
    QCOMPARE(greenRgb, QColor(Qt::green));

    const int yellowAnsi = rgbToAnsi256(255, 255, 0);
    QCOMPARE(yellowAnsi, 226);
    const QColor yellowRgb = mmqt::ansi256toRgb(yellowAnsi);
    QCOMPARE(yellowRgb, QColor(Qt::yellow));

    const int blueAnsi = rgbToAnsi256(0, 0, 255);
    QCOMPARE(blueAnsi, 21);
    const QColor blueRgb = mmqt::ansi256toRgb(blueAnsi);
    QCOMPARE(blueRgb, QColor(Qt::blue));

    const int magentaAnsi = rgbToAnsi256(255, 0, 255);
    QCOMPARE(magentaAnsi, 201);
    const QColor magentaRgb = mmqt::ansi256toRgb(magentaAnsi);
    QCOMPARE(magentaRgb, QColor(Qt::magenta));

    const int cyanAnsi = rgbToAnsi256(0, 255, 255);
    QCOMPARE(cyanAnsi, 51);
    const QColor cyanRgb = mmqt::ansi256toRgb(cyanAnsi);
    QCOMPARE(cyanRgb, QColor(Qt::cyan));

    const int whiteAnsi = rgbToAnsi256(255, 255, 255);
    QCOMPARE(whiteAnsi, 231);
    const QColor whiteRgb = mmqt::ansi256toRgb(whiteAnsi);
    QCOMPARE(whiteRgb, QColor(Qt::white));

    const int grayAnsi = rgbToAnsi256(128, 128, 128);
    QCOMPARE(grayAnsi, 244);
    const QColor grayRgb = mmqt::ansi256toRgb(grayAnsi);
    QCOMPARE(grayRgb, QColor(Qt::darkGray));

    // these are called for the side-effect of testing their asserts.
#define X_TEST(N, lower, UPPER) \
    do { \
        const std::string_view testing = #lower; \
        std::ignore = mmqt::rgbToAnsi256String(lower##Rgb, AnsiColor16LocationEnum::Foreground); \
        std::ignore = mmqt::rgbToAnsi256String(lower##Rgb, AnsiColor16LocationEnum::Background); \
    } while (false);
    XFOREACH_ANSI_COLOR_0_7(X_TEST)

    // TODO: use colons instead of semicolons
    QCOMPARE(mmqt::rgbToAnsi256String(blackRgb, AnsiColor16LocationEnum::Foreground),
             QString("[38;5;16m"));
    QCOMPARE(mmqt::rgbToAnsi256String(blackRgb, AnsiColor16LocationEnum::Background),
             QString("[37;48;5;16m"));

    QCOMPARE(mmqt::rgbToAnsi256String(whiteRgb, AnsiColor16LocationEnum::Foreground),
             QString("[38;5;231m"));
    QCOMPARE(mmqt::rgbToAnsi256String(whiteRgb, AnsiColor16LocationEnum::Background),
             QString("[30;48;5;231m"));
#undef X_TEST
}

void TestGlobal::ansiOstreamTest()
{
    test::testAnsiOstream();
}

void TestGlobal::ansiTextUtilsTest()
{
    mmqt::HideQDebug forThisTest;
    test::testAnsiTextUtils();
}

void TestGlobal::ansiToRgbTest()
{
    static_assert(153 == 16 + 36 * 3 + 6 * 4 + 5);
    // ansi_rgb6(3x4x5) is light blue with a lot of green, it's definitely not cyan.
    //
    // https://en.wikipedia.org/wiki/ANSI_escape_code#SGR_(Select_Graphic_Rendition)_parameters
    // lists 153 as #afd7ff, which looks light blue.
    // If you're looking for cyan, try 159 (#afffff).
    const int cyanAnsi = 153;
    const QColor cyanRgb = mmqt::ansi256toRgb(cyanAnsi);
    QCOMPARE(cyanRgb, QColor("#AFD7FF"));

    auto testOne = [](const int ansi256, const QColor &color, const AnsiColor16Enum ansi) {
        QCOMPARE(mmqt::toQColor(ansi), color);
        QCOMPARE(mmqt::ansi256toRgb(ansi256), color);
    };

    testOne(0, "#2E3436", AnsiColor16Enum::black);
    testOne(6, "#06989A", AnsiColor16Enum::cyan);
    testOne(7, "#D3D7CF", AnsiColor16Enum::white);

    testOne(8, "#555753", AnsiColor16Enum::BLACK);
    testOne(14, "#34E2E2", AnsiColor16Enum::CYAN);
    testOne(15, "#EEEEEC", AnsiColor16Enum::WHITE);
}

void TestGlobal::caseUtilsTest()
{
    test::testCaseUtils();
}

void TestGlobal::castTest()
{
    test::test_int_cast();
    test::test_float_cast();
}

void TestGlobal::charsetTest()
{
    test::testCharset();
}

void TestGlobal::charUtilsTest()
{
    test::testCharUtils();
}

void TestGlobal::colorTest()
{
    test::testColor();
}

void TestGlobal::diffTest()
{
    mmqt::HideQDebug forThisTest;
    test::testDiff();
}

void TestGlobal::emojiTest()
{
    test::test_emojis();
}

void TestGlobal::entitiesTest()
{
    test::test_entities();
}

void TestGlobal::flagsTest()
{
    test::testFlags();
}

void TestGlobal::hideQDebugTest()
{
    static constexpr auto onlyDebug = []() {
        mmqt::HideQDebugOptions tmp;
        tmp.hideInfo = false;
        return tmp;
    }();

    static constexpr auto onlyInfo = []() {
        mmqt::HideQDebugOptions tmp;
        tmp.hideDebug = false;
        return tmp;
    }();

    const QString expected = "1{DIW}\n2{DW}\n3{DIW}\n4{IW}\n5{DIW}\n"
                             "---\n"
                             "1{W}\n2{W}\n3{W}\n4{W}\n5{W}\n"
                             "---\n"
                             "1{DIW}\n2{DW}\n3{DIW}\n4{IW}\n5{DIW}\n";

    static QString tmp;
    static QString expectMsg;

    auto testCase = [](int n) {
        expectMsg = QString::asprintf("%d", n);
        tmp += expectMsg;
        tmp += "{";
        qDebug() << n;
        qInfo() << n;
        qWarning() << n;
        tmp += "}\n";
    };

    auto testAlternations = [&testCase]() {
        testCase(1);
        {
            mmqt::HideQDebug hide{onlyInfo};
            testCase(2);
        }
        testCase(3);
        {
            mmqt::HideQDebug hide{onlyDebug};
            testCase(4);
        }
        testCase(5);
    };
    static auto localMessageHandler =
        [](const QtMsgType type, const QMessageLogContext &, const QString &msg) {
            assert(expectMsg == msg);
            switch (type) {
            case QtDebugMsg:
                tmp += "D";
                break;
            case QtWarningMsg:
                tmp += "W";
                break;
            case QtCriticalMsg:
                tmp += "C";
                break;
            case QtFatalMsg:
                tmp += "F";
                break;
            case QtInfoMsg:
                tmp += "I";
                break;
            }
        };

    tmp.clear();
    {
        const auto old = qInstallMessageHandler(localMessageHandler);
        RAIICallback restoreOld{[old]() { qInstallMessageHandler(old); }};

        {
            testAlternations();
            tmp += "---\n";
            {
                mmqt::HideQDebug hideBoth;
                testAlternations();
            }
            tmp += "---\n";
            testAlternations();
        }
    }
    QCOMPARE(tmp, expected);

    // new case: warnings can also be disabled.
    const QString expected2 = "1{DI}\n2{D}\n3{DI}\n4{I}\n5{DI}\n";
    tmp.clear();
    {
        const auto old = qInstallMessageHandler(localMessageHandler);
        RAIICallback restoreOld{[old]() { qInstallMessageHandler(old); }};
        mmqt::HideQDebug inThisScope{mmqt::HideQDebugOptions{false, false, true}};
        testAlternations();
    }
    QCOMPARE(tmp, expected2);
}

void TestGlobal::indexedVectorWithDefaultTest()
{
    test::testIndexedVectorWithDefault();
}

void TestGlobal::lineUtilsTest()
{
    test::testLineUtils();
}

namespace { // anonymous

// Multiple signals can share the same lifetime
void sig2_test_disconnects()
{
    const std::vector<int> expected = {1, 2, 2, 3, 2, 3};
    std::vector<int> order;

    Signal2<> sig;
    std::optional<Signal2Lifetime> opt_lifetime;
    opt_lifetime.emplace();

    size_t calls = 0;

    QCOMPARE(sig.getNumConnected(), 0);
    sig.connect(opt_lifetime.value(), [&calls, &order]() {
        ++calls;
        order.push_back(1);
    });
    QCOMPARE(sig.getNumConnected(), 1);
    QCOMPARE(calls, 0);

    sig();
    QCOMPARE(calls, 1);
    QCOMPARE(sig.getNumConnected(), 1); // the object doesn't know that #1 will on the next call.

    opt_lifetime.reset();
    sig();
    QCOMPARE(calls, 1);
    QCOMPARE(sig.getNumConnected(), 0); // now it knows
    opt_lifetime.emplace();
    QCOMPARE(sig.getNumConnected(), 0); // creating a new lifetime doesn't reconnect.

    sig();
    QCOMPARE(calls, 1);
    QCOMPARE(sig.getNumConnected(), 0);

    size_t calls2 = 0;
    sig.connect(opt_lifetime.value(), [&calls2, &order]() {
        ++calls2;
        order.push_back(2);
    });
    QCOMPARE(calls2, 0);
    QCOMPARE(sig.getNumConnected(), 1);

    sig();
    QCOMPARE(calls, 1);
    QCOMPARE(calls2, 1);
    QCOMPARE(sig.getNumConnected(), 1);

    size_t calls3 = 0;
    sig.connect(opt_lifetime.value(), [&calls3, &order]() {
        ++calls3;
        order.push_back(3);
    });
    QCOMPARE(calls, 1);
    QCOMPARE(calls2, 1);
    QCOMPARE(calls3, 0);
    QCOMPARE(sig.getNumConnected(), 2);

    sig();
    QCOMPARE(calls, 1);
    QCOMPARE(calls2, 2);
    QCOMPARE(calls3, 1);
    QCOMPARE(sig.getNumConnected(), 2);

    sig();
    QCOMPARE(calls, 1);
    QCOMPARE(calls2, 3);
    QCOMPARE(calls3, 2);
    QCOMPARE(sig.getNumConnected(), 2);

    opt_lifetime.reset();
    sig();
    QCOMPARE(calls, 1);
    QCOMPARE(calls2, 3);
    QCOMPARE(calls3, 2);
    QCOMPARE(sig.getNumConnected(), 0);

    QCOMPARE(order, expected);
}

// Exceptions disable signals and allow other signals to execute.
void sig2_test_exceptions()
{
    Signal2<> sig;
    std::optional<Signal2Lifetime> opt_lifetime;
    opt_lifetime.emplace();

    const std::vector<int> expected = {1, 2, 2};
    std::vector<int> order;
    size_t calls = 0;
    sig.connect(opt_lifetime.value(), [&calls, &order]() {
        ++calls;
        order.push_back(1);
        throw std::runtime_error("on purpose");
    });
    size_t calls2 = 0;
    sig.connect(opt_lifetime.value(), [&calls2, &order]() {
        ++calls2;
        order.push_back(2);
    });

    QCOMPARE(calls, 0);
    QCOMPARE(calls2, 0);
    QCOMPARE(sig.getNumConnected(), 2);

    {
        // hide the warning about the purposely-thrown exception
        mmqt::HideQDebug inThisScope{mmqt::HideQDebugOptions{true, true, true}};
        sig();
    }
    QCOMPARE(calls, 1);
    QCOMPARE(calls2, 1);
    QCOMPARE(sig.getNumConnected(), 1); // exception immediately removed it

    sig();
    QCOMPARE(calls, 1);
    QCOMPARE(calls2, 2);
    QCOMPARE(sig.getNumConnected(), 1);

    QCOMPARE(order, expected);
}

void sig2_test_recursion()
{
    Signal2<> sig;
    std::optional<Signal2Lifetime> opt_lifetime;
    opt_lifetime.emplace();

    const std::vector<int> expected = {1, 2, 2};
    std::vector<int> order;
    size_t calls = 0;
    sig.connect(opt_lifetime.value(), [&calls, &order, &sig]() {
        ++calls;
        order.push_back(1);
        sig();
    });
    {
        // hide the warning about the recursion exception
        mmqt::HideQDebug inThisScope{mmqt::HideQDebugOptions{true, true, true}};
        sig();
    }
    QCOMPARE(calls, 1);
    QCOMPARE(sig.getNumConnected(), 0);
    sig();
    QCOMPARE(calls, 1);
    QCOMPARE(sig.getNumConnected(), 0);
}

} // namespace

void TestGlobal::signal2Test()
{
    sig2_test_disconnects();
    sig2_test_exceptions();
    sig2_test_recursion();
}

void TestGlobal::stringViewTest()
{
    test::testStringView();
}

void TestGlobal::taggedStringTest()
{
    test::testTaggedString();
}

void TestGlobal::textUtilsTest()
{
    test::testTextUtils();
}

void TestGlobal::toLowerLatin1Test()
{
    QCOMPARE(toLowerLatin1(char(0xC0u)), char(0xE0u));
    QCOMPARE(toLowerLatin1(char(0xDDu)), char(0xFDu));
    QCOMPARE(toLowerLatin1(char(0xDEu)), char(0xFEu));

    // before the range of letters
    QCOMPARE(toLowerLatin1(char(0xBFu)), char(0xBFu)); // inverted question mark

    // inside the range of letters:
    QCOMPARE(toLowerLatin1(char(0xD7u)), char(0xD7u)); // multiplication sign
    QCOMPARE(toLowerLatin1(char(0xF7u)), char(0xF7u)); // division sign

    // special cases:
    QCOMPARE(toLowerLatin1(char(0xDFu)), char(0xDFu)); // lowercase sharp s
    QCOMPARE(toLowerLatin1(char(0xFFu)), char(0xFFu)); // lowercase y with two dots

    QCOMPARE(toLowerLatin1('A'), 'a');
    QCOMPARE(toLowerLatin1('Z'), 'z');

    using char_consts::C_MINUS_SIGN;
    QCOMPARE(toLowerLatin1(C_MINUS_SIGN), C_MINUS_SIGN);

    {
        int num_lower_latin1 = 0;
        int num_upper_latin1 = 0;
        int num_lower_utf8 = 0;
        int num_upper_utf8 = 0;

        for (int i = 0; i < 256; ++i) {
            const auto c = static_cast<char>(static_cast<uint8_t>(i));
            num_lower_latin1 += isLowerLatin1(c);
            num_upper_latin1 += isUpperLatin1(c);
            assert(!isLowerLatin1(c) || !isUpperLatin1(c));

            const char latin1[2]{c, char_consts::C_NUL};
            const auto utf8 = charset::conversion::latin1ToUtf8(latin1);

            num_lower_utf8 += containsLowerUtf8(utf8);
            num_upper_utf8 += containsUpperUtf8(utf8);
            assert(!containsLowerUtf8(utf8) || !containsUpperUtf8(utf8));

            assert(isLowerLatin1(c) == containsLowerLatin1(latin1));
            assert(isLowerLatin1(c) == containsLowerUtf8(utf8));

            assert(isUpperLatin1(c) == containsUpperLatin1(latin1));
            assert(isUpperLatin1(c) == containsUpperUtf8(utf8));
        }

        QCOMPARE(num_lower_latin1, 26 + 30);
        QCOMPARE(num_upper_latin1, 26 + 30);
        QCOMPARE(num_lower_utf8, 26 + 30);
        QCOMPARE(num_upper_utf8, 26 + 30);
    }

    {
        QCOMPARE(toLowerLatin1('A'), 'a');
        QCOMPARE(toLowerLatin1('a'), 'a');
        QCOMPARE(toLowerLatin1(std::string("A")), "a");
        QCOMPARE(toLowerLatin1(std::string("a")), "a");
        QCOMPARE(toLowerLatin1(std::string_view("A")), "a");
        QCOMPARE(toLowerLatin1(std::string_view("a")), "a");

        QCOMPARE(toUpperLatin1('A'), 'A');
        QCOMPARE(toUpperLatin1('a'), 'A');
        QCOMPARE(toUpperLatin1(std::string("A")), "A");
        QCOMPARE(toUpperLatin1(std::string("a")), "A");
        QCOMPARE(toUpperLatin1(std::string_view("A")), "A");
        QCOMPARE(toUpperLatin1(std::string_view("a")), "A");

        const std::string s = "Abc\xCF\xDF\xEF\xFF"; // testing latin1
        const auto v = std::string_view(s);

        QCOMPARE(toLowerLatin1(s), "abc\xEF\xDF\xEF\xFF");
        QCOMPARE(toLowerLatin1(v), "abc\xEF\xDF\xEF\xFF");

        QCOMPARE(toUpperLatin1(s), "ABC\xCF\xDF\xCF\xFF");
        QCOMPARE(toUpperLatin1(v), "ABC\xCF\xDF\xCF\xFF");
    }
    {
        QCOMPARE(toLowerUtf8('A'), 'a');
        QCOMPARE(toLowerUtf8('a'), 'a');
        QCOMPARE(toLowerUtf8(std::string("A")), "a");
        QCOMPARE(toLowerUtf8(std::string("a")), "a");
        QCOMPARE(toLowerUtf8(std::string_view("A")), "a");
        QCOMPARE(toLowerUtf8(std::string_view("a")), "a");

        QCOMPARE(toUpperUtf8('A'), 'A');
        QCOMPARE(toUpperUtf8('a'), 'A');
        QCOMPARE(toUpperUtf8(std::string("A")), "A");
        QCOMPARE(toUpperUtf8(std::string("a")), "A");
        QCOMPARE(toUpperUtf8(std::string_view("A")), "A");
        QCOMPARE(toUpperUtf8(std::string_view("a")), "A");

        const std::string s = "Abc\u00CF\u00DF\u00EF\u00FF"; // testing utf8
        const auto v = std::string_view(s);

        QCOMPARE(toLowerUtf8(s), "abc\u00EF\u00DF\u00EF\u00FF");
        QCOMPARE(toLowerUtf8(v), "abc\u00EF\u00DF\u00EF\u00FF");

        QCOMPARE(toUpperUtf8(s), "ABC\u00CF\u00DF\u00CF\u00FF");
        QCOMPARE(toUpperUtf8(v), "ABC\u00CF\u00DF\u00CF\u00FF");
    }
}

void TestGlobal::toNumberTest()
{
    QCOMPARE(to_integer<uint64_t>(u"0"), 0);
    QCOMPARE(to_integer<uint64_t>(u"1"), 1);
    QCOMPARE(to_integer<uint64_t>(u"1234567890"), 1234567890);
    QCOMPARE(to_integer<uint64_t>(u"12345678901234567890"), 12345678901234567890ull);
    QCOMPARE(to_integer<uint64_t>(u"18446744073709551615"), 18446744073709551615ull);
    QCOMPARE(to_integer<uint64_t>(u"18446744073709551616").has_value(), false);
    QCOMPARE(to_integer<uint64_t>(u"36893488147419103231").has_value(), false);
    QCOMPARE(to_integer<uint64_t>(u"92233720368547758079").has_value(), false);
    QCOMPARE(to_integer<uint64_t>(u"110680464442257309695").has_value(), false);
}

void TestGlobal::unquoteTest()
{
    test::testUnquote();
}

void TestGlobal::weakHandleTest()
{
    test::testWeakHandle();
}

QTEST_MAIN(TestGlobal)
