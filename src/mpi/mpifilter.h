#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/Consts.h"
#include "../global/Signal2.h"
#include "../mpi/remoteeditsession.h"
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
    void onEditMessage(const RemoteSession &session, const QString &title, const QString &body)
    {
        virt_onEditMessage(session, title, body);
    }
    void onViewMessage(const QString &title, const QString &body)
    {
        virt_onViewMessage(title, body);
    }

private:
    virtual void virt_onParseNewMudInput(const TelnetData &) = 0;
    virtual void virt_onEditMessage(const RemoteSession &, const QString &, const QString &) = 0;
    virtual void virt_onViewMessage(const QString &, const QString &) = 0;
};

// from Mud
class NODISCARD MpiFilter final
{
private:
    MpiFilterOutputs &m_outputs;
    RawBytes m_buffer;
    int m_remaining = 0;
    TelnetDataEnum m_previousType = TelnetDataEnum::Empty;
    char m_command = char_consts::C_NUL;
    bool m_receivingMpi = false;

public:
    explicit MpiFilter(MpiFilterOutputs &outputs)
        : m_outputs{outputs}
    {}

protected:
    void parseMessage(char command, const RawBytes &buffer);
    void parseEditMessage(const RawBytes &buffer);
    void parseViewMessage(const RawBytes &buffer);

public:
    void analyzeNewMudInput(const TelnetData &data);
};

// toMud
class NODISCARD MpiFilterToMud
{
public:
    MpiFilterToMud() = default;
    virtual ~MpiFilterToMud();

public:
    void cancelRemoteEdit(const RemoteEditMessageBytes &sessionId);
    void saveRemoteEdit(const RemoteEditMessageBytes &sessionId, const Latin1Bytes &content);

private:
    void submitMpi(const RawBytes &bytes);
    virtual void virt_submitMpi(const RawBytes &bytes) = 0;
};

NODISCARD extern bool isMpiMessage(const RawBytes &bytes);
NODISCARD extern bool hasMpiPrefix(const QString &s);
