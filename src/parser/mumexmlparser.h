#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <optional>
#include <QByteArray>
#include <QString>
#include <QtCore/QFile>
#include <QtCore>
#include <QtGlobal>

#include "CommandId.h"
#include "LineFlags.h"
#include "abstractparser.h"

class MapData;
class MumeClock;
class ProxyParserApi;
class GroupManagerApi;
class QDataStream;
class QFile;
class QObject;
struct IncomingData;

// #define XMLPARSER_STREAM_DEBUG_INPUT_TO_FILE

class MumeXmlParser : public AbstractParser
{
    Q_OBJECT

public:
    explicit MumeXmlParser(
        MapData *, MumeClock *, ProxyParserApi, GroupManagerApi, QObject *parent = nullptr);
    ~MumeXmlParser() override;

    void parse(const IncomingData &);

public slots:
    void parseNewMudInput(const IncomingData &data) override;

private:
    QDataStream *debugStream = nullptr;
    QFile *file = nullptr;

private:
    static const QByteArray greaterThanChar;
    static const QByteArray lessThanChar;
    static const QByteArray greaterThanTemplate;
    static const QByteArray lessThanTemplate;
    static const QByteArray ampersand;
    static const QByteArray ampersandTemplate;

private:
    void parseMudCommands(const QString &str);

private:
    QByteArray characters(QByteArray &ch);
    bool element(const QByteArray &);

private:
    CommandEnum m_move = CommandEnum::LOOK;
    void move();

private:
    enum class XmlModeEnum { NONE, ROOM, NAME, DESCRIPTION, EXITS, PROMPT, TERRAIN, HEADER };
    XmlModeEnum m_xmlMode = XmlModeEnum::NONE;
    LineFlags m_lineFlags;

    QByteArray m_lineToUser;
    QByteArray m_tempCharacters;
    QByteArray m_tempTag;
    QString m_stringBuffer;
    bool m_readingTag = false;
    bool m_gratuitous = false;
    bool m_exitsReady = false;
    bool m_descriptionReady = false;

    // REVISIT: Is there any point to having a distinction between null and empty?
    std::optional<RoomName> m_roomName;
    std::optional<RoomStaticDesc> m_staticRoomDesc;
    std::optional<RoomDynamicDesc> m_dynamicRoomDesc;

private:
    void stripXmlEntities(QByteArray &ch);
};
