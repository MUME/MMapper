// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors
// Author: Mike Repass <mike.repass@gmail.com> (Taryn)

#include "gameobserver.h"

#include "../global/parserutils.h"

void GameObserver::observeConnected()
{
    resetKnownRooms();
    sig2_connected.invoke();
}

void GameObserver::observeSentToMud(const QString &input)
{
    auto str = input;
    ParserUtils::removeAnsiMarksInPlace(str);
    sig2_sentToMudString.invoke(str);
}

void GameObserver::observeSentToUser(const QString &input)
{
    auto str = input;
    ParserUtils::removeAnsiMarksInPlace(str);
    sig2_sentToUserString.invoke(str);
}

void GameObserver::observeSentToUserGmcp(const GmcpMessage &m)
{
    const auto revision = getKnownRooms().revision;
    const bool request = m_knownRooms.receive(m);
    knownRoomsChanged(revision, request);
    sig2_sentToUserGmcp.invoke(m);
}

void GameObserver::observeToggledEchoMode(const bool echo)
{
    sig2_toggledEchoMode.invoke(echo);
}

void GameObserver::observeWeather(const PromptWeatherEnum weather)
{
    if (m_weather != weather) {
        m_weather = weather;
        sig2_weatherChanged.invoke(weather);
    }
}

void GameObserver::observeFog(const PromptFogEnum fog)
{
    if (m_fog != fog) {
        m_fog = fog;
        sig2_fogChanged.invoke(fog);
    }
}

void GameObserver::observeTimeOfDay(MumeTimeEnum timeOfDay)
{
    if (m_timeOfDay != timeOfDay) {
        m_timeOfDay = timeOfDay;
        sig2_timeOfDayChanged.invoke(m_timeOfDay);
    }
}

void GameObserver::observeMoonPhase(MumeMoonPhaseEnum moonPhase)
{
    if (m_moonPhase != moonPhase) {
        m_moonPhase = moonPhase;
        sig2_moonPhaseChanged.invoke(m_moonPhase);
    }
}

void GameObserver::observeMoonVisibility(MumeMoonVisibilityEnum moonVisibility)
{
    if (m_moonVisibility != moonVisibility) {
        m_moonVisibility = moonVisibility;
        sig2_moonVisibilityChanged.invoke(m_moonVisibility);
    }
}

void GameObserver::observeSeason(MumeSeasonEnum season)
{
    if (m_season != season) {
        m_season = season;
        sig2_seasonChanged.invoke(m_season);
    }
}

void GameObserver::knownRoomsChanged(const uint64_t previousRevision, const bool request)
{
    if (previousRevision != getKnownRooms().revision) {
        sig2_knownRoomsChanged.invoke();
    }
    if (request) {
        sig2_requestKnownRooms.invoke();
    }
}

void GameObserver::resetKnownRooms()
{
    const auto revision = getKnownRooms().revision;
    m_knownRooms.reset();
    knownRoomsChanged(revision, false);
}

void GameObserver::enableKnownRooms(const bool enabled)
{
    const auto revision = getKnownRooms().revision;
    const bool request = m_knownRooms.setEnabled(enabled);
    knownRoomsChanged(revision, request);
}

void GameObserver::configureKnownRoomsCache(const QString &directory, const QString &server)
{
    const auto revision = getKnownRooms().revision;
    m_knownRooms.configureCache(
        directory,
        server,
        [this]() { sig2_knownRoomsChanged.invoke(); },
        [this]() { sig2_knownRoomsCacheMissing.invoke(); });
    knownRoomsChanged(revision, false);
}
