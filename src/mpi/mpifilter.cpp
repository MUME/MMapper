// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "mpifilter.h"

#include "../configuration/configuration.h"
#include "../proxy/GmcpMessage.h"
#include "../proxy/telnetfilter.h"

#include <QByteArray>
#include <QMessageLogContext>
#include <QObject>
#include <QString>

namespace { // anonymous
volatile const bool verbose_debugging = false;
} // namespace

MpiFilterOutputs::~MpiFilterOutputs() = default;

void MpiFilter::receiveMpiView(const QString &title, const QString body)
{
    m_outputs.onViewMessage(title, body);
}

void MpiFilter::receiveMpiEdit(const RemoteSessionId id, const QString &title, const QString body)
{
    m_outputs.onEditMessage(id, title, body);
}

MpiFilterToMud::~MpiFilterToMud() = default;

void MpiFilterToMud::sendRemoteEditMessage(GmcpMessageTypeEnum type,
                                           const RemoteSessionId id,
                                           const QString &text = QString())
{
    QJsonObject obj;
    if (!text.isNull()) {
        // TODO: add + QString("\u0100") to test failure
        obj["text"] = text;
    }
    obj["id"] = id.asInt32();
    QJsonDocument doc;
    doc.setObject(obj);
    GmcpJson json{QString::fromUtf8(doc.toJson())};
    GmcpMessage msg{type, json};
    if (verbose_debugging) {
        qInfo() << msg.toRawBytes();
    }
    virt_submitGmcp(msg);
}

void MpiFilterToMud::cancelRemoteEdit(const RemoteSessionId id)
{
    sendRemoteEditMessage(GmcpMessageTypeEnum::MUME_CLIENT_CANCEL_EDIT, id);
}

void MpiFilterToMud::saveRemoteEdit(const RemoteSessionId id, const Latin1Bytes &content)
{
    auto payload = content.getQByteArray();
    QString text = QString::fromLatin1(payload);
    sendRemoteEditMessage(GmcpMessageTypeEnum::MUME_CLIENT_WRITE, id, text);
}

void MpiFilterToMud::submitGmcp(const GmcpMessage &gmcpMessage)
{
    virt_submitGmcp(gmcpMessage);
}
