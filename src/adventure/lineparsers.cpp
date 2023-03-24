// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors
// Author: Mike Repass <mike.repass@gmail.com> (Taryn)

#include "lineparsers.h"

// required for warning -W non-virtual-dtor
AbstractLineParser::~AbstractLineParser() {}

QString AbstractLineParser::getLastSuccessVal()
{
    return m_lastSuccessVal;
}

bool AccomplishedTaskParser::parse(QString line)
{
    // REVISIT: there are at least three different versions of this
    //   accomplished
    //   knowledgeable (xp?)
    //   travelled (tp?)
    return line.startsWith("With the task complete, you feel more");
}

bool AchievementParser::parse(QString line)
{
    // An achievement event is:
    //   (1) A line matching exactly "You achieved something new!"
    //   (2) The next line is interpreted as the achievement text.

    if (m_pending) {
        m_lastSuccessVal = line.trimmed();
        m_pending = false;
        return true;
    }

    m_pending = line.startsWith("You achieved something new!");

    return false;
}

bool DiedParser::parse(QString line)
{
    return line.startsWith("You are dead! Sorry...");
}

bool GainedLevelParser::parse(QString line)
{
    return line.startsWith("You gain a level!");
}

bool HintParser::parse(QString line)
{
    // A hint event is:
    //   (1) A line matching exactly "# Hint:"
    //   (2) The next line is interpreted as the hint text.
    if (m_pending) {
        m_lastSuccessVal = line.mid(4).trimmed(); // mid(4) is to chomp the leading "#  "
        m_pending = false;
        return true;
    }

    m_pending = line.startsWith("# Hint:");
    return false;
}

bool KillAndXPParser::parse(QString line)
{
    // A kill and exp earned event as follows:
    //   (1) A line exactly matching "You receive your share of experience."
    // ... and then within the next 5 lines, a line:
    //   (2a) matching " is dead! R.I.P."
    //   (2b) OR matching " disappears into nothing."

    if (line.startsWith("You receive your share of experience.")) {
        m_pending = true;
        m_linesSinceShareExp = 0;
        return false;
    }

    if (!m_pending)
        return false;

    m_linesSinceShareExp++;

    if (m_linesSinceShareExp > 5) {
        // too many lines have passed, our pending share exp has expired
        m_pending = false;
        return false;

    } else {
        // We're in a pending share of experience, let's see if a kill
        auto idx_dead = std::max(line.indexOf(" is dead! R.I.P."),
                                 line.indexOf(" disappears into nothing."));

        if (idx_dead > -1) {
            // Kill, extract the mob/enemy name and reset pending
            m_lastSuccessVal = line.left(idx_dead);
            m_pending = false;
            return true;
        }

        return false;
    }
}
