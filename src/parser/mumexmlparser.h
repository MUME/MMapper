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

#include "../pandoragroup/mmapper2character.h"
#include "CommandId.h"
#include "abstractparser.h"

class MapData;
class MumeClock;
class ProxyParserApi;
class QDataStream;
class QFile;
class QObject;
struct IncomingData;

// #define XMLPARSER_STREAM_DEBUG_INPUT_TO_FILE

enum class XmlModeEnum { NONE, ROOM, NAME, DESCRIPTION, EXITS, PROMPT, TERRAIN, HEADER };

class MumeXmlParser : public AbstractParser
{
    Q_OBJECT

public:
    explicit MumeXmlParser(MapData *, MumeClock *, ProxyParserApi, QObject *parent = nullptr);
    ~MumeXmlParser() override;

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

    CommandEnum m_move = CommandEnum::LOOK;
    XmlModeEnum m_xmlMode = XmlModeEnum::NONE;

    void move();
    QByteArray m_lineToUser;
    QByteArray m_tempCharacters;
    QByteArray m_tempTag;
    QString m_stringBuffer;
    bool m_readingTag = false;
    bool m_readStatusTag = false;
    bool m_readWeatherTag = false;
    bool m_gratuitous = false;
    bool m_readSnoopTag = false;
    bool m_exitsReady = false;
    bool m_descriptionReady = false;

    // REVISIT: Is there any point to having a distinction between null and empty?
    std::optional<RoomName> m_roomName;
    std::optional<RoomStaticDesc> m_staticRoomDesc;
    std::optional<RoomDynamicDesc> m_dynamicRoomDesc;

private:
    void stripXmlEntities(QByteArray &ch);

signals:
    void sendScoreLineEvent(QByteArray);
    void sendPromptLineEvent(QByteArray);
    void mumeTime(QString);
    void sendCharacterPositionEvent(CharacterPositionEnum);
    void sendCharacterAffectEvent(CharacterAffectEnum, bool);
};
