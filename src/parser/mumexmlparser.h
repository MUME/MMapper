#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <optional>
#include <string_view>
#include <QByteArray>
#include <QString>
#include <QtCore/QFile>
#include <QtCore>
#include <QtGlobal>

#include "CommandId.h"
#include "LineFlags.h"
#include "abstractparser.h"

class GroupManagerApi;
class MapData;
class MumeClock;
class ProxyParserApi;
class QDataStream;
class QFile;
class QObject;
struct TelnetData;

// #define XMLPARSER_STREAM_DEBUG_INPUT_TO_FILE

enum class NODISCARD XmlModeEnum { NONE, ROOM, NAME, DESCRIPTION, EXITS, PROMPT, TERRAIN, HEADER };

class MumeXmlParser final : public AbstractParser
{
private:
    Q_OBJECT

private:
    QDataStream *debugStream = nullptr;
    QFile *file = nullptr;
    XmlModeEnum m_xmlMode = XmlModeEnum::NONE;
    LineFlags m_lineFlags;
    QByteArray m_lineToUser;
    QByteArray m_tempCharacters;
    QByteArray m_tempTag;
    QString m_stringBuffer;
    std::optional<char> m_snoopChar;
    bool m_readingTag = false;
    bool m_gratuitous = false;
    bool m_exitsReady = false;
    bool m_descriptionReady = false;

    // REVISIT: Is there any point to having a distinction between null and empty?
    std::optional<RoomName> m_roomName;
    std::optional<RoomDesc> m_roomDesc;
    std::optional<RoomContents> m_roomContents;

public:
    explicit MumeXmlParser(MapData &, MumeClock &, ProxyParserApi, GroupManagerApi, QObject *parent);
    ~MumeXmlParser() final;

private:
    void parse(const TelnetData &);

public:
    void slot_parseNewMudInput(const TelnetData &data);

private:
    void parseMudCommands(const QString &str);
    NODISCARD QByteArray characters(QByteArray &ch);
    NODISCARD bool element(const QByteArray &);
    void move();
    NODISCARD std::string snoopToUser(const std::string_view &str);

private:
    static void stripXmlEntities(QByteArray &ch);
};
