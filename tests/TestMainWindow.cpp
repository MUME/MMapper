// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "TestMainWindow.h"

#include "../src/configuration/configuration.h"
#include "../src/display/MapCanvasConfig.h"
#include "../src/display/MapScroller.h"
#include "../src/mainwindow/AudioVolumeSlider.h"
#include "../src/mainwindow/CompactLayout.h"
#include "../src/mainwindow/UpdateChecker.h"
#include "../src/map/coordinate.h"

#include <QDebug>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QtTest/QtTest>

const char *getMMapperVersion()
{
    return "v19.04.0-72-ga16c196";
}

const char *getMMapperBranch()
{
    return "master";
}

bool isMMapperBeta()
{
    return false;
}

TestMainWindow::TestMainWindow() = default;

TestMainWindow::~TestMainWindow() = default;

void TestMainWindow::updaterTest()
{
    CompareVersion version{QString::fromUtf8(getMMapperVersion())};
    QVERIFY2(version == version, "Version compared to itself matches");

    CompareVersion current{"2.8.0"};
    QVERIFY2((current > current) == false, "Current is not greater than itself");

    CompareVersion newerMajor{"19.04.0"};
    QVERIFY2(newerMajor > current, "Newer major version is greater than older version");
    QVERIFY2((current > newerMajor) == false, "Older version is not newer than newer major");

    CompareVersion newerMinor{"2.9.0"};
    QVERIFY2(newerMinor > current, "Newer major version is greater than older version");
    QVERIFY2((current > newerMinor) == false, "Older version is not newer than newer minor version");

    CompareVersion newerPatch{"2.9.0"};
    QVERIFY2(newerPatch > current, "Newer major version is greater than older version");
    QVERIFY2((current > newerPatch) == false, "Older version is not newer than newer patch version");
}

void TestMainWindow::audioToolbarTest()
{
    setEnteredMain();

    const int originalMusic = getConfig().audio.getMusicVolume();
    const int originalSound = getConfig().audio.getSoundVolume();

    // Ensure we restore configuration
    auto cleanup = qScopeGuard([=]() {
        setConfig().audio.setMusicVolume(originalMusic);
        setConfig().audio.setSoundVolume(originalSound);
    });

    AudioVolumeSlider musicSlider(AudioVolumeSlider::AudioType::Music);
    AudioVolumeSlider soundSlider(AudioVolumeSlider::AudioType::Sound);

    // Initial value should match current config defaults
    QCOMPARE(musicSlider.value(), getConfig().audio.getMusicVolume());
    QCOMPARE(soundSlider.value(), getConfig().audio.getSoundVolume());

    // Update config -> slider updates
    setConfig().audio.setMusicVolume(75);
    QCOMPARE(musicSlider.value(), 75);

    setConfig().audio.setSoundVolume(25);
    QCOMPARE(soundSlider.value(), 25);

    // Update slider -> config updates
    musicSlider.setValue(90);
    QCOMPARE(getConfig().audio.getMusicVolume(), 90);

    soundSlider.setValue(10);
    QCOMPARE(getConfig().audio.getSoundVolume(), 10);
}

void TestMainWindow::mapScrollerScrollMath()
{
    // Regression test for MapScroller's world<->scroll-unit conversion
    // (see MapScroller.h/.cpp), which mirrors MapWindow's known map bounds.
    MapScroller scroller;

    // A 10x6-unit known map (mirrors MapData::sig_mapSizeChanged's
    // min/max Coordinates, which MapScroller::slot_setScrollBars()
    // receives).
    const Coordinate min{-2, -3, 0};
    const Coordinate max{8, 3, 0};

    scroller.slot_setScrollBars(min, max);

    // size() is (max - min) = (10, 6); range is size * SCROLL_SCALE.
    QCOMPARE(scroller.getHorizontalScrollMax(), 10 * MapCanvasConfig::SCROLL_SCALE);
    QCOMPARE(scroller.getVerticalScrollMax(), 6 * MapCanvasConfig::SCROLL_SCALE);

    // worldToScroll() must invert scrollToWorld() (modulo the integer
    // rounding baked into scroll units).
    const glm::vec2 world{1.0f, 2.0f};
    const glm::ivec2 scroll = scroller.worldToScroll(world);
    const glm::vec2 roundTripped = scroller.scrollToWorld(scroll);
    QVERIFY(std::abs(roundTripped.x - world.x) < 0.01f);
    QVERIFY(std::abs(roundTripped.y - world.y) < 0.01f);

    // Spot-check an exact known value: world == min maps to scroll (0, dims.y)
    // (Y is negated/flipped -- see KnownMapSize::worldToScroll()'s comment).
    const glm::ivec2 scrollAtMin = scroller.worldToScroll(glm::vec2{min.x, min.y});
    QCOMPARE(scrollAtMin.x, 0);
    QCOMPARE(scrollAtMin.y, 6 * MapCanvasConfig::SCROLL_SCALE);

    // ... and world == max maps to scroll (dims.x, 0).
    const glm::ivec2 scrollAtMax = scroller.worldToScroll(glm::vec2{max.x, max.y});
    QCOMPARE(scrollAtMax.x, 10 * MapCanvasConfig::SCROLL_SCALE);
    QCOMPARE(scrollAtMax.y, 0);

    // Continuous-scroll timer: starting a non-zero step must fire
    // sig_continuousScrollStep() periodically until stopped.
    QSignalSpy stepSpy(&scroller, &MapScroller::sig_continuousScrollStep);
    scroller.slot_continuousScroll(1, 0);
    QTRY_VERIFY_WITH_TIMEOUT(!stepSpy.isEmpty(), 2000);
    QCOMPARE(stepSpy.constLast().at(0).toInt(), 1);
    QCOMPARE(stepSpy.constLast().at(1).toInt(), 0);

    scroller.slot_continuousScroll(0, 0);
    stepSpy.clear();
    QTest::qWait(250);
    QCOMPARE(stepSpy.count(), 0);
}

void TestMainWindow::compactLayoutPolicy()
{
    using namespace CompactLayout;

    // Exactly at the breakpoint is not compact; one pixel short on either
    // axis is.
    QVERIFY(!isCompact(QSize(MIN_WIDTH, MIN_HEIGHT)));
    QVERIFY(!isCompact(QSize(1920, 1080)));
    QVERIFY(isCompact(QSize(MIN_WIDTH - 1, MIN_HEIGHT)));
    QVERIFY(isCompact(QSize(MIN_WIDTH, MIN_HEIGHT - 1)));
    // Landscape phone: wide enough but too short.
    QVERIFY(isCompact(QSize(800, 360)));

    QCOMPARE(clientHeight(QSize(400, 1000)), 550);
    QCOMPARE(clientHeight(QSize(400, 0)), 0);

    // A window the expanded layout's minimum size has pushed past a phone's
    // screen is judged by the screen, so the compact layout still engages.
    QCOMPARE(probeSize(QSize(1080, 900), QSize(390, 664)), QSize(390, 664));
    QVERIFY(isCompact(probeSize(QSize(1080, 900), QSize(390, 664))));
    // A window smaller than its screen is judged by itself.
    QCOMPARE(probeSize(QSize(600, 420), QSize(1920, 1080)), QSize(600, 420));
    // No screen: the window size stands.
    QCOMPARE(probeSize(QSize(1080, 900), QSize()), QSize(1080, 900));
}

QTEST_MAIN(TestMainWindow)
