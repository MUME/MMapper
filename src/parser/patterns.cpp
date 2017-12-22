/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor)
**
** This file is part of the MMapper project. 
** Maintained by Nils Schimmelmann <nschimme@gmail.com>
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the:
** Free Software Foundation, Inc.
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
************************************************************************/

#include "patterns.h"
#include "configuration.h"

QRegExp Patterns::m_rx;

const QRegExp Patterns::m_score("\\d+/\\d+ hits(?:, \\d+/\\d+ mana,)? and \\d+/\\d+ moves.");

// Used for oldRoom loading only
QStringList Patterns::m_dynamicDescriptionPatternsList(
        QStringList()
        << "#!(?:A|An|The)[^.]*\\."
        << "#![^.]*(?:sit(?:s|ting)?|rest(?:s|ing)?|stand(?:s|ing)|sleep(?:s|ing)?|swim(?:s|ming)?|walk(?:s|ing)?|wander(?:s|ing)?|grow(?:s|ing)?|lies?|lying)[^.]*"
        << "#!.*(?:\\(glowing\\)|\\(shrouded\\)|\\(hidden\\))\\.?"
        << "#![^.]*(in|on) the ground\\."
        << "#?*"
        << "#<You have found"
        << "#<You have revealed"
        << "#>whip all around you."
        << "#<Clusters of"
        << "#<Prickly"
        << "#!.*arrived from.*\\.");

bool Patterns::matchPattern(QString pattern, QString& str)
{
	if (pattern.at(0) != '#')
		return false;
	
    switch ((int)(pattern.at(1)).toLatin1())
	{
	case 33:  // !
		m_rx.setPattern(pattern.remove(0,2));
		if (m_rx.exactMatch(str)) 
			return true;
		break;
	case 60:;  // <
		if (str.startsWith(pattern.remove(0,2))) 
			return true;
		break;
	case 61:;  // =
		if ( str==(pattern.remove(0,2)) ) 
			return true;
		break;
	case 62:;  // >
		if (str.endsWith(pattern.remove(0,2))) 
			return true;
		break;
	case 63:;  // ?
		if (str.contains(pattern.remove(0,2))) 
			return true;
		break;
	default:;
	}
	return false;	
}

bool Patterns::matchPattern(QByteArray pattern, QByteArray& str)
{
	if (pattern.at(0) != '#')
		return false;
	
	switch ((int)(pattern.at(1)))
	{
	case 33:  // !
		break;
	case 60:;  // <
		if (str.startsWith(pattern.remove(0,2))) 
			return true;
		break;
	case 61:;  // =
		if ( str==(pattern.remove(0,2)) ) 
			return true;
		break;
	case 62:;  // >
		if (str.endsWith(pattern.remove(0,2))) 
			return true;
		break;
	case 63:;  // ?
		if (str.contains(pattern.remove(0,2))) 
			return true;
		break;
	default:;
	}
	return false;	
}

bool Patterns::matchScore(QString& str)
{
    return m_score.exactMatch(str);
}

bool Patterns::matchMoveForcePatterns(QString& str)
{
	for ( QStringList::iterator it = Config().m_moveForcePatternsList.begin(); 
		it != Config().m_moveForcePatternsList.end(); ++it ) 
		if (matchPattern(*it, str)) return true;
	return false;
}

bool Patterns::matchNoDescriptionPatterns(QString& str)
{
	for ( QStringList::iterator it = Config().m_noDescriptionPatternsList.begin(); 
		it != Config().m_noDescriptionPatternsList.end(); ++it ) 
		if (matchPattern(*it, str)) return true;
	return false;
}

bool Patterns::matchDynamicDescriptionPatterns(QString& str)
{
    for (QStringList::iterator it = m_dynamicDescriptionPatternsList.begin();
        it != m_dynamicDescriptionPatternsList.end(); ++it )
		if (matchPattern(*it, str)) return true;
    return false;
}

bool Patterns::matchPasswordPatterns(QByteArray& str)
{	
	if (matchPattern(Config().m_passwordPattern, str)) return true;
	return false;
}

bool Patterns::matchPromptPatterns(QByteArray& str)
{
	if (matchPattern(Config().m_promptPattern, str)) return true;
	return false;
}

bool Patterns::matchLoginPatterns(QByteArray& str)
{
	if (matchPattern(Config().m_loginPattern, str)) return true;
	return false;
}

bool Patterns::matchMenuPromptPatterns(QByteArray& str)
{
	if (matchPattern(Config().m_menuPromptPattern, str)) return true;
	return false;
}

