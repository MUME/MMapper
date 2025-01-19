#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../map/CommandId.h"
#include "LineFlags.h"
#include "abstractparser.h"

#include <optional>
#include <string_view>

#include <QByteArray>
#include <QString>
#include <QtCore/QFile>
#include <QtCore>
#include <QtGlobal>

class GmcpMessage;
class GroupManagerApi;
class MapData;
class MumeClock;
class ProxyParserApi;
class QDataStream;
class QFile;
class QObject;
struct TelnetData;

// #define XMLPARSER_STREAM_DEBUG_INPUT_TO_FILE

enum class NODISCARD XmlModeEnum : uint8_t {
    NONE,
    ROOM,
    NAME,
    DESCRIPTION,
    EXITS,
    PROMPT,
    TERRAIN,
    HEADER,
    CHARACTER
};

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

private:
    enum class NODISCARD XmlAttributeStateEnum : uint8_t {
        /// received <room/>
        ELEMENT,
        /// received <room terrain/>
        ATTRIBUTE,
        /// received <room terrain=/>
        EQUALS,
        /// received <room terrain=field/>
        UNQUOTED_VALUE,
        /// received <room terrain='field'/>
        SINGLE_QUOTED_VALUE,
        /// received <room terrain="field"/>
        DOUBLE_QUOTED_VALUE
    };

public:
    explicit MumeXmlParser(
        MapData &, MumeClock &, ProxyParserApi, GroupManagerApi, CTimers &timers, QObject *parent);
    ~MumeXmlParser() final;

private:
    void parse(const TelnetData &, bool isGoAhead);

public:
    void slot_parseNewMudInput(const TelnetData &data);
    void slot_parseGmcpInput(const GmcpMessage &msg);

private:
    void parseMudCommands(const QString &str);
    NODISCARD QByteArray characters(QByteArray &ch);
    NODISCARD bool element(const QByteArray &);
    void move();
    NODISCARD std::string snoopToUser(std::string_view str);

private:
    static void stripXmlEntities(QByteArray &ch);
};
