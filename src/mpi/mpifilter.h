#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/Consts.h"
#include "../global/Signal2.h"
#include "../mpi/remoteeditsession.h"
#include "../proxy/GmcpMessage.h"
#include "../proxy/telnetfilter.h"

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QtCore>

struct NODISCARD MpiFilterOutputs
{
public:
    virtual ~MpiFilterOutputs();

public:
    void onParseNewMudInput(const TelnetData &data) { virt_onParseNewMudInput(data); }
    void onEditMessage(const RemoteSessionId id, const QString &title, const QString &body)
    {
        virt_onEditMessage(id, title, body);
    }
    void onViewMessage(const QString &title, const QString &body)
    {
        virt_onViewMessage(title, body);
    }

private:
    virtual void virt_onParseNewMudInput(const TelnetData &) = 0;
    virtual void virt_onEditMessage(const RemoteSessionId, const QString &, const QString &) = 0;
    virtual void virt_onViewMessage(const QString &, const QString &) = 0;
};

// from Mud
class NODISCARD MpiFilter final
{
private:
    MpiFilterOutputs &m_outputs;

public:
    explicit MpiFilter(MpiFilterOutputs &outputs)
        : m_outputs{outputs}
    {}

private:
    void parseMessage(char command, const RawBytes &buffer);
    void parseEditMessage(const RawBytes &buffer);
    void parseViewMessage(const RawBytes &buffer);

public:
    void receiveMpiView(const QString &title, const QString body);
    void receiveMpiEdit(const RemoteSessionId id, const QString &title, const QString body);
};

// toMud
class NODISCARD MpiFilterToMud
{
public:
    MpiFilterToMud() = default;
    virtual ~MpiFilterToMud();

public:
    void cancelRemoteEdit(const RemoteSessionId id);
    void saveRemoteEdit(const RemoteSessionId id, const Latin1Bytes &content);

private:
    void sendRemoteEditMessage(GmcpMessageTypeEnum type,
                               const RemoteSessionId id,
                               const QString &text);
    void submitGmcp(const GmcpMessage &gmcpMessage);
    virtual void virt_submitGmcp(const GmcpMessage &gmcpMessage) = 0;
};
