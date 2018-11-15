#pragma once
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

#ifndef MUMEXMLPARSER_H
#define MUMEXMLPARSER_H

#include <QByteArray>
#include <QRegExp>
#include <QString>
#include <QtCore/QFile>
#include <QtCore>
#include <QtGlobal>

#include "CommandId.h"
#include "abstractparser.h"

class MapData;
class MumeClock;
class QDataStream;
class QFile;
class QObject;
struct IncomingData;

// #define XMLPARSER_STREAM_DEBUG_INPUT_TO_FILE

enum class XmlMode { NONE, ROOM, NAME, DESCRIPTION, EXITS, PROMPT, TERRAIN };

class MumeXmlParser : public AbstractParser
{
    Q_OBJECT

public:
    explicit MumeXmlParser(MapData *, MumeClock *, QObject *parent = nullptr);
    ~MumeXmlParser();

    void parse(const IncomingData &);

public slots:
    void parseNewMudInput(const IncomingData &data) override;

protected:
    QDataStream *debugStream = nullptr;
    QFile *file = nullptr;

    static const QByteArray greaterThanChar;
    static const QByteArray lessThanChar;
    static const QByteArray greaterThanTemplate;
    static const QByteArray lessThanTemplate;
    static const QByteArray ampersand;
    static const QByteArray ampersandTemplate;

    void parseMudCommands(const QString &str);

    QByteArray characters(QByteArray &ch);
    bool element(const QByteArray &);

    CommandIdType m_move = CommandIdType::LOOK;
    XmlMode m_xmlMode = XmlMode::NONE;

    void move();
    QByteArray m_lineToUser{};
    QByteArray m_tempCharacters{};
    QByteArray m_tempTag{};
    QString m_stringBuffer{};
    bool m_readingTag = false;
    bool m_readStatusTag = false;
    bool m_readWeatherTag = false;
    bool m_gratuitous = false;
    bool m_readSnoopTag = false;
    bool m_readingRoomDesc = false;
    bool m_descriptionReady = false;

    QString m_roomName = nullString;
    QString m_staticRoomDesc = nullString;
    QString m_dynamicRoomDesc = nullString;

private:
    void stripXmlEntities(QByteArray &ch);

signals:
    void sendScoreLineEvent(QByteArray);
    void sendPromptLineEvent(QByteArray);
    void mumeTime(QString);
};

#endif
