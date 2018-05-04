/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor),
**            Nils Schimmelmann <nschimme@gmail.com> (Jahara)
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

#include "configuration.h"
#include <QSettings>
#include <QHostInfo>
#include <QFontDatabase>

Configuration::Configuration()
{
    read(); // read the settings or set them to the default values
}

void Configuration::read()
{
    QSettings conf("Caligor soft", "MMapper2");

    conf.beginGroup("General");
    m_firstRun = conf.value("Run first time", true).toBool();
    windowPosition = conf.value("Window Position", QPoint(200, 200)).toPoint();
    windowSize = conf.value("Window Size", QSize(800, 600)).toSize();
    windowState = conf.value("Window State", "").toByteArray();
    alwaysOnTop = conf.value("Always On Top", false).toBool();
    m_mapMode = conf.value("Map Mode", 0).toInt(); //0 play, 1 map
    conf.endGroup();

    // read general options
    conf.beginGroup("Connection");
    m_remoteServerName = conf.value("Server name", "mume.org").toString();
    m_remotePort = conf.value("Remote port number", 4242).toInt();
    m_localPort = conf.value("Local port number", 4242).toInt();
    conf.endGroup();

    // News 2340, changing domain from fire.pvv.org to mume.org:
    if (m_remoteServerName.contains("pvv.org")) {
        m_remoteServerName = "mume.org";
    }

    conf.beginGroup("Canvas");
    m_showUpdated = conf.value("Show updated rooms", false).toBool();
    m_drawNotMappedExits = conf.value("Draw not mapped exits", true).toBool();
    m_drawNoMatchExits = conf.value("Draw no match exits", false).toBool();
    m_drawUpperLayersTextured = conf.value("Draw upper layers textured", false).toBool();
    m_drawDoorNames = conf.value("Draw door names", true).toBool();
    m_backgroundColor = QColor(conf.value("Background color", QColor(110, 110, 110).name()).toString());
    m_antialiasingSamples = conf.value("Number of anti-aliasing samples", 0).toInt();
    m_trilinearFiltering = conf.value("Use trilinear filtering", false).toBool();
    m_softwareOpenGL = conf.value("Use software OpenGL", false).toBool();
    conf.endGroup();

    conf.beginGroup("Auto load world");
    m_autoLoadWorld = conf.value("Auto load", false ).toBool();
    m_autoLoadFileName = conf.value("File name", "arda.mm2").toString();
    m_lastMapDirectory = conf.value("Last map load directory",
#if defined(Q_OS_LINUX)
                                    "/usr/share/games/mmapper"
#elif defined(Q_OS_WIN)
                                    "C:/Program Files (x86)/MMapper"
#else
                                    ""
#endif
                                   ).toString();
    conf.endGroup();

    conf.beginGroup("Parser");
    m_roomNameColor = conf.value("Room name ansi color", "[32m").toString();
    m_roomDescColor = conf.value("Room desc ansi color", "[0m").toString();

    m_removeXmlTags = conf.value("Remove XML tags", true).toBool();

    m_moveForcePatternsList = conf.value("Move force patterns for XML").toStringList();
    m_noDescriptionPatternsList = conf.value("Non-standard room description patterns").toStringList();
    m_promptPattern = conf.value("Prompt pattern", "#>>").toByteArray();
    m_loginPattern = conf.value("Login pattern", "#>known? ").toByteArray();
    m_passwordPattern = conf.value("Password pattern", "#>pass phrase: ").toByteArray();
    m_menuPromptPattern = conf.value("Menu prompt pattern", "#>> ").toByteArray();

    // XML mode used UTF-8, non-XML used Latin1.
    m_utf8Charset = conf.value("MUME charset is UTF-8", false).toBool();

    conf.endGroup();

    conf.beginGroup("Mume client protocol");
    m_IAC_prompt_parser = conf.value("Use Telnet IAC-GA prompts", true).toBool();
    m_remoteEditing = conf.value("Remote editing and viewing", true).toBool();
    m_internalRemoteEditor = conf.value("Use internal editor", true).toBool();
    m_externalRemoteEditorCommand = conf.value("External editor command",
#if defined(Q_OS_LINUX)
                                               "gedit"
#elif defined(Q_OS_WIN)
                                               "notepad"
#elif defined(Q_OS_MAC)
                                               "open -t"
#else
                                               ""
#endif
                                              ).toString();
    conf.endGroup();

    conf.beginGroup("Mume native");
    m_emulatedExits = conf.value("Emulated Exits", true).toBool();
    m_showHiddenExitFlags = conf.value("Show hidden exit flags", true).toBool();
    m_showNotes = conf.value("Show notes", true).toBool();
    conf.endGroup();

    if (m_moveForcePatternsList.isEmpty()) {
        m_moveForcePatternsList.append("#?leads you out");
        m_moveForcePatternsList.append("#<Suddenly an explosion of ancient rhymes");
        m_moveForcePatternsList.append("#=You board the ferry."); // Grey Havens
        m_moveForcePatternsList.append("#=You leave the ferry.");
    }

    if (m_noDescriptionPatternsList.isEmpty()) {
        m_noDescriptionPatternsList.append("#=It is pitch black...");
        m_noDescriptionPatternsList.append("#=You just see a dense fog around you...");
    }

    conf.beginGroup("Path Machine");
    m_acceptBestRelative = conf.value("relative path acceptance", 25).toDouble();
    m_acceptBestAbsolute = conf.value("absolute path acceptance", 6).toDouble();
    m_newRoomPenalty = conf.value("room creation penalty", 5).toDouble();
    m_correctPositionBonus = conf.value("correct position bonus", 5).toDouble();
    m_multipleConnectionsPenalty = conf.value("multiple connections penalty", 2.0).toDouble();
    m_maxPaths = conf.value("maximum number of paths", 1000).toUInt();
    m_matchingTolerance = conf.value("room matching tolerance", 8).toUInt();
    conf.endGroup();

    conf.beginGroup("Group Manager");
    m_groupManagerState = conf.value("state", 0).toInt(); // OFF
    m_groupManagerLocalPort = conf.value("local port", 4243).toInt();
    m_groupManagerRemotePort = conf.value("remote port", 4243).toInt();
    m_groupManagerHost = conf.value("host", "localhost").toByteArray();
    m_groupManagerCharName = conf.value("character name", QHostInfo::localHostName()).toByteArray();
    m_groupManagerShareSelf = conf.value("share self", true).toBool();
    m_groupManagerColor = (QColor) conf.value("color", "#ffff00").toString();
    m_groupManagerRulesWarning = conf.value("rules warning", true).toBool();
    conf.endGroup();

    conf.beginGroup("Mume Clock");
    m_mumeStartEpoch = conf.value("Mume start epoch", 1517443173).toInt();
    m_displayMumeClock = conf.value("Display clock", true).toBool();
    conf.endGroup();

    conf.beginGroup("Integrated Mud Client");
    QFont defaultClientFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    defaultClientFont.setPointSize(12);
    defaultClientFont.setStyleHint(QFont::Monospace, QFont::PreferAntialias);
    m_clientFont.fromString(conf.value("Font", defaultClientFont.toString()).toString());
    m_clientBackgroundColor = conf.value("Background color", QColor(Qt::black).name()).toString();
    m_clientForegroundColor = conf.value("Foreground color", QColor(Qt::lightGray).name()).toString();
    m_clientColumns = conf.value("Columns", 80).toInt();
    m_clientRows = conf.value("Rows", 24).toInt();
    m_clientLinesOfScrollback = conf.value("Lines of scrollback", 10000).toInt();
    m_clientLinesOfInputHistory = conf.value("Lines of input history", 100).toInt();
    m_clientTabCompletionDictionarySize = conf.value("Tab completion dictionary size", 100).toInt();
    m_clientClearInputOnEnter = conf.value("Clear input on enter", true).toBool();
    conf.endGroup();
}

void Configuration::write() const
{

    QSettings conf("Caligor soft", "MMapper2");

    conf.beginGroup("General");
    conf.setValue("Run first time", false);
    conf.setValue("Window Position", windowPosition);
    conf.setValue("Window Size", windowSize);
    conf.setValue("Window State", windowState);
    conf.setValue("Always On Top", alwaysOnTop);
    conf.setValue("Map Mode", m_mapMode);
    conf.endGroup();

    // write general options
    conf.beginGroup("Connection");
    conf.setValue("Server name", m_remoteServerName);
    conf.setValue("Remote port number", (int) m_remotePort);
    conf.setValue("Local port number", (int) m_localPort);
    conf.endGroup();

    // write style options
    conf.beginGroup("Canvas");
    conf.setValue("Show updated rooms", m_showUpdated);
    conf.setValue("Draw not mapped exits", m_drawNotMappedExits);
    conf.setValue("Draw no match exits", m_drawNoMatchExits);
    conf.setValue("Draw upper layers textured", m_drawUpperLayersTextured);
    conf.setValue("Draw door names", m_drawDoorNames);
    conf.setValue("Background color", m_backgroundColor.name());
    conf.setValue("Number of anti-aliasing samples", m_antialiasingSamples);
    conf.setValue("Use trilinear filtering", m_trilinearFiltering);
    conf.setValue("Use software OpenGL", m_softwareOpenGL);
    conf.endGroup();

    conf.beginGroup("Auto load world");
    conf.setValue("Auto load", m_autoLoadWorld);
    conf.setValue("File name", m_autoLoadFileName);
    conf.setValue("Last map load directory", m_lastMapDirectory);
    conf.endGroup();

    conf.beginGroup("Parser");
    conf.setValue("Room name ansi color", m_roomNameColor);
    conf.setValue("Room desc ansi color", m_roomDescColor);

    conf.setValue("Remove XML tags", m_removeXmlTags);

    conf.setValue("Move force patterns for XML", m_moveForcePatternsList);
    conf.setValue("No room description patterns", m_noDescriptionPatternsList);
    conf.setValue("Prompt pattern", m_promptPattern);
    conf.setValue("Login pattern", m_loginPattern);
    conf.setValue("Password pattern", m_passwordPattern);
    conf.setValue("Menu prompt pattern", m_menuPromptPattern);

    conf.setValue("MUME charset is UTF-8", m_utf8Charset);
    conf.endGroup();

    conf.beginGroup("Mume native");
    conf.setValue("Emulated Exits", m_emulatedExits);
    conf.setValue("Show hidden exit flags", m_showHiddenExitFlags);
    conf.setValue("Show notes", m_showNotes);
    conf.endGroup();

    conf.beginGroup("Mume client protocol");
    conf.setValue("Use Telnet IAC-GA prompts", m_IAC_prompt_parser);
    conf.setValue("Remote editing and viewing", m_remoteEditing);
    conf.setValue("Use internal editor", m_internalRemoteEditor);
    conf.setValue("External editor command", m_externalRemoteEditorCommand);
    conf.endGroup();

    conf.beginGroup("Path Machine");
    conf.setValue("relative path acceptance", m_acceptBestRelative);
    conf.setValue("absolute path acceptance", m_acceptBestAbsolute);
    conf.setValue("room creation penalty", m_newRoomPenalty);
    conf.setValue("correct position bonus", m_correctPositionBonus);
    conf.setValue("maximum number of paths", m_maxPaths);
    conf.setValue("room matching tolerance", m_matchingTolerance);
    conf.setValue("multiple connections penalty", m_multipleConnectionsPenalty);
    conf.endGroup();

    conf.beginGroup("Group Manager");
    conf.setValue("state", m_groupManagerState);
    conf.setValue("local port", m_groupManagerLocalPort);
    conf.setValue("remote port", m_groupManagerRemotePort);
    conf.setValue("host", m_groupManagerHost);
    conf.setValue("character name", m_groupManagerCharName);
    conf.setValue("share self", m_groupManagerShareSelf);
    conf.setValue("color", m_groupManagerColor.name());
    conf.setValue("rules warning", m_groupManagerRulesWarning);
    conf.endGroup();

    conf.beginGroup("Mume Clock");
    conf.setValue("Mume start epoch", m_mumeStartEpoch);
    conf.setValue("Display clock", m_displayMumeClock);
    conf.endGroup();

    conf.beginGroup("Integrated Mud Client");
    conf.setValue("Font", m_clientFont.toString());
    conf.setValue("Background color", m_clientBackgroundColor.name());
    conf.setValue("Foreground color", m_clientForegroundColor.name());
    conf.setValue("Columns", m_clientColumns);
    conf.setValue("Rows", m_clientRows);
    conf.setValue("Lines of scrollback", m_clientLinesOfScrollback);
    conf.setValue("Lines of input history", m_clientLinesOfInputHistory);
    conf.setValue("Tab completion dictionary size", m_clientTabCompletionDictionarySize);
    conf.setValue("Clear input on enter", m_clientClearInputOnEnter);
    conf.endGroup();
}

Configuration &Config()
{
    static Configuration conf;
    return conf;
}

bool Configuration::isChanged() const
{
    return configurationChanged;
}
