// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors
// Author: Mike Repass <mike.repass@gmail.com> (Taryn)

#include "lineparsers.h"

#include <QDebug>

bool AccomplishedTaskParser::parse(const QString &line)
{
    // REVISIT: there are at least three different versions of this
    //   accomplished
    //   knowledgeable (xp?)
    //   travelled (tp?)
    // (In that case, plan to change to return a LineParserResult to indicate which one it was...)
    return line.startsWith("With the task complete, you feel more");
}

LineParserResult AchievementParser::parse(const QString &prev, const QString &line)
{
    // An achievement event is:
    //   (1) A line matching exactly "You achieved something new!"
    //   (2) The next line is interpreted as the achievement text.

    if (!prev.startsWith("You achieved something new!")) {
        return std::nullopt;
    }

    return line.trimmed();
}

bool DiedParser::parse(const QString &line)
{
    return line.startsWith("You are dead! Sorry...");
}

bool GainedLevelParser::parse(const QString &line)
{
    return line.startsWith("You gain a level!");
}

LineParserResult HintParser::parse(const QString &prev, const QString &line)
{
    // A hint event is:
    //   (1) A line matching exactly "# Hint:"
    //   (2) The next line is interpreted as the hint text.
    if (!prev.startsWith("# Hint:")) {
        return std::nullopt;
    }

    // Consider using a regex here to allow variation in number of spaces, or do something like:
    // trim, check for #, remove it, trim again, and then verify that the hint isn't "Hint:",
    // which could happen if the line "# Hint:" is somehow repeated twice.
    if (line.length() < 4 || !line.startsWith("#   ")) {
        qWarning() << "Hint has unexpected format.";
        return std::nullopt;
    }
    return line.mid(4).trimmed(); // mid(4) is to chomp the leading "#  "
}

LineParserResult KillAndXPParser::parse(const QString &line)
{
    // A kill and exp earned event as follows:
    // A line matching exactly either of:
    //   (1a) "You receive your share of experience." (mob)
    //   (1b) "You feel more experienced." (player)
    // And then within the next _5_ lines, a line:
    // For mob kill:
    //   (2a) matching " is dead! R.I.P."
    //   (2b) OR matching " disappears into nothing."
    // For player kill:
    //   (3a) matching " has drawn his last breath! R.I.P."
    //   (3a) matching " has drawn her last breath! R.I.P."

    if (line.startsWith("You receive your share of experience.")
        || line.startsWith("You feel more experienced.")) {
        m_pending = true;
        m_linesSinceShareExp = 0;
        return std::nullopt;
    }

    if (!m_pending) {
        return std::nullopt;
    }

    m_linesSinceShareExp++;

    if (m_linesSinceShareExp > 5) {
        // too many lines have passed, our pending share exp has expired
        m_pending = false;
        return std::nullopt;
    }
    // We're in a pending share of experience, let's see if a kill
    auto idx_dead = line.indexOf(" is dead! R.I.P.");
    if (idx_dead == -1) {
        idx_dead = line.indexOf(" disappears into nothing.");
    }

    if (idx_dead > -1) {
        // Kill, extract the mob/enemy name and reset pending
        m_lastSuccessVal = line.left(idx_dead);
        m_pending = false;
        return m_lastSuccessVal;
    }

    idx_dead = line.indexOf(" has drawn his last breath! R.I.P.");
    if (idx_dead == -1) {
        idx_dead = line.indexOf(" has drawn her last breath! R.I.P.");
    }

    if (idx_dead > -1) {
        // Kill, extract the mob/enemy name and reset pending
        m_lastSuccessVal = line.left(idx_dead);
        m_pending = false;
        return m_lastSuccessVal;
    }

    return std::nullopt;
}
