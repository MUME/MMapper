// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "UserTelnet.h"
#include "../configuration/configuration.h"
#include "../global/TextUtils.h"

#include <sstream>
#include <QJsonDocument>

UserTelnet::UserTelnet(QObject *const parent)
    : AbstractTelnet(TextCodecStrategyEnum::AUTO_SELECT_CODEC, false, parent)
{}

void UserTelnet::onConnected()
{
    reset();
    // Negotiate options
    requestTelnetOption(TN_DO, OPT_TERMINAL_TYPE);
    requestTelnetOption(TN_DO, OPT_NAWS);
    requestTelnetOption(TN_DO, OPT_CHARSET);
    // Most clients expect the server (i.e. MMapper) to send IAC WILL GMCP
    requestTelnetOption(TN_WILL, OPT_GMCP);
    // Request permission to replace IAC GA with IAC EOR
    requestTelnetOption(TN_WILL, OPT_EOR);
}

void UserTelnet::onAnalyzeUserStream(const QByteArray &data)
{
    onReadInternal(data);
}

void UserTelnet::onSendToUser(const QByteArray &ba, const bool goAhead)
{
    // MMapper internally represents all data as Latin-1
    QString temp = QString::fromLatin1(ba);

    // Convert from unicode into the client requested encoding
    QByteArray outdata = getTextCodec().fromUnicode(temp);

    submitOverTelnet(outdata, goAhead);
}

void UserTelnet::onGmcpToUser(const GmcpMessage &msg)
{
    if (myOptionState[OPT_GMCP])
        sendGmcpMessage(msg);
}

void UserTelnet::sendToMapper(const QByteArray &data, const bool goAhead)
{
    // MMapper requires all data to be Latin-1 internally
    // REVISIT: This will break things if the client is handling MPI itself
    QByteArray outdata = getTextCodec().toUnicode(data).toLatin1();
    emit analyzeUserStream(outdata, goAhead);
}

void UserTelnet::onRelayEchoMode(const bool isDisabled)
{
    sendTelnetOption(isDisabled ? TN_WONT : TN_WILL, OPT_ECHO);
    myOptionState[OPT_ECHO] = !isDisabled;
    announcedState[OPT_ECHO] = true;
}

void UserTelnet::receiveGmcpMessage(const GmcpMessage &msg)
{
    // Eat Core.Hello since MMapper sends its own to MUME
    if (msg.isCoreHello())
        return;

    // Eat Core.Supports.[Add|Set|Remove] and proxy a MMapper filtered subset
    if (msg.getJson()
        && (msg.isCoreSupportsAdd() || msg.isCoreSupportsSet() || msg.isCoreSupportsRemove())) {
        // Deserialize the payload
        QJsonDocument doc = QJsonDocument::fromJson(msg.getJson()->toQByteArray());
        if (!doc.isArray())
            return;
        const auto &array = doc.array();

        // Handle the various messages
        if (msg.isCoreSupportsSet())
            resetGmcpModules();
        for (const auto &e : array) {
            if (!e.isString())
                continue;
            const auto &moduleStr = e.toString();
            try {
                const GmcpModule module(moduleStr);
                receiveGmcpModule(module, !msg.isCoreSupportsRemove());

            } catch (const std::exception &e) {
                qWarning() << "Module" << moduleStr
                           << (msg.isCoreSupportsRemove() ? "remove" : "add")
                           << "error because:" << e.what();
            }
        }

        // Filter MMapper-internal GMCP modules to proxy on to MUME
        std::ostringstream oss;
        oss << "[ ";
        bool comma = false;
        for (auto &module : gmcp.modules) {
            // REVISIT: Are some MMapper supported modules not supposed to be filtered?
            if (module.isSupported())
                continue;
            if (comma)
                oss << ", ";
            oss << '"' << module.toStdString() << '"';
            comma = true;
        }
        oss << " ]";
        GmcpMessage filteredMsg(GmcpMessageTypeEnum::CORE_SUPPORTS_SET, oss.str());
        emit relayGmcp(filteredMsg);
        return;
    }

    emit relayGmcp(msg);
}

void UserTelnet::receiveTerminalType(const QByteArray &data)
{
    emit relayTermType(data);
}

void UserTelnet::receiveWindowSize(const int x, const int y)
{
    emit relayNaws(x, y);
}

/** Send out the data. Does not double IACs, this must be done
            by caller if needed. This function is suitable for sending
            telnet sequences. */
void UserTelnet::sendRawData(const QByteArray &data)
{
    sentBytes += data.length();
    emit sendToSocket(data);
}
