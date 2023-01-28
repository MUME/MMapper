#include "lineparsers.h"

AbstractStatefulLineParser::~AbstractStatefulLineParser() {}

QString AbstractStatefulLineParser::getLastSuccessVal()
{
    return m_lastSuccessVal;
}

bool AchievementParser::parse(QString line)
{
    // An achivement event is:
    //   (1) A line matching exactly "You achieved something new!"
    //   (2) The next line is interpreted as the achievement text.

    if (m_pending) {
        m_lastSuccessVal = line;
        m_pending = false;
        return true;
    }

    m_pending = (line.indexOf("You achieved something new!") == 0);

    return false;
}

bool DiedParser::parse(QString line)
{
    return (line.indexOf("You are dead! Sorry...") == 0);
}

bool GainedLevelParser::parse(QString line)
{
    return (line.indexOf("You gain a level!") == 0);
}

bool HintParser::parse(QString line)
{
    // A hint event is:
    //   (1) A line matching exactly "# Hint:"
    //   (2) The next line is interpreted as the hint text.
    if (m_pending) {
        m_lastSuccessVal = line.mid(4); // Chomp the leading "#  "
        m_pending = false;
        return true;
    }

    m_pending = (line.indexOf("# Hint:") == 0);
    return false;
}

bool KillAndXPParser::parse(QString line)
{
    // A kill and exp earned event as follows:
    //   (1) A line exactly matching "You receive your share of experience."
    // ... and then within the next 5 lines, a line:
    //   (2a) matching " is dead! R.I.P."
    //   (2b) OR matching " disappears into nothing."

    if (line.indexOf("You receive your share of experience.") == 0) {
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

bool TaskCompleteParser::parse(QString line)
{
    // REVISIT: there are at least three different versions of this
    //   accomplished
    //   knowledgeable (xp?)
    //   travelled (tp?)
    return line.indexOf("With the task complete, you feel more") == 0;
}
