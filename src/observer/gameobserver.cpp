// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors
// Author: Mike Repass <mike.repass@gmail.com> (Taryn)

#include "gameobserver.h"

#include "../global/parserutils.h"

void GameObserver::observeConnected()
{
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
