// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "UserTelnet.h"

#include <sstream>
#include <QJsonDocument>

#include "../global/Charset.h"
#include "../global/TextUtils.h"

// REVISIT: Does this belong somewhere else?
static void normalizeForUser(std::ostream &os,
                             const CharacterEncodingEnum encoding,
                             const bool goAhead,
                             const std::string_view sv)
{
    // REVISIT: perform ANSI normalization in this function, too?
    foreachLine(sv, [&os, encoding, &goAhead](std::string_view sv) {
        if (sv.empty())
            return;

        const bool hasNewline = sv.back() == C_NEWLINE;
        if (hasNewline) {
            sv.remove_suffix(1);
        }

        if (!sv.empty() && sv.back() == C_CARRIAGE_RETURN) {
            sv.remove_suffix(1);
        }

        if (!sv.empty()) {
            foreachChar(sv, C_CARRIAGE_RETURN, [&os, encoding, &goAhead](std::string_view txt) {
                if (!txt.empty() && (txt.front() != C_CARRIAGE_RETURN || goAhead)) {
                    convertFromLatin1(os, encoding, txt);
                }
            });
        }

        if (hasNewline) {
            os << C_CARRIAGE_RETURN;
            os << C_NEWLINE;
        }
    });
}

NODISCARD static std::string encodeForUser(const CharacterEncodingEnum encoding,
                                           const QByteArray &ba,
                                           const bool goAhead)
{
    std::ostringstream oss;
    normalizeForUser(oss, encoding, goAhead, ::toStdStringViewLatin1(ba));
    return oss.str();
}

NODISCARD static QByteArray decodeFromUser(const CharacterEncodingEnum encoding,
                                           const QByteArray &ba)
{
    switch (encoding) {
    case CharacterEncodingEnum::ASCII:
        return ba;
    case CharacterEncodingEnum::LATIN1:
        return ba;
    case CharacterEncodingEnum::UTF8: {
        std::ostringstream oss;
        for (const QChar qc : QString::fromUtf8(ba)) {
            const auto codepoint = qc.unicode();
            if (codepoint < 256) {
                oss << static_cast<char>(codepoint & 0xFF);
            } else
                oss << "?";
        }
        return ::toQByteArrayLatin1(oss.str());
    }
    default:
        break;
    }
    abort();
}

UserTelnet::UserTelnet(QObject *const parent)
    : AbstractTelnet(TextCodecStrategyEnum::AUTO_SELECT_CODEC, parent, "unknown")
{}

void UserTelnet::slot_onConnected()
{
    reset();
    resetGmcpModules();

    // Negotiate options
    requestTelnetOption(TN_DO, OPT_TERMINAL_TYPE);
    requestTelnetOption(TN_DO, OPT_NAWS);
    requestTelnetOption(TN_DO, OPT_CHARSET);
    // Most clients expect the server (i.e. MMapper) to send IAC WILL GMCP
    requestTelnetOption(TN_WILL, OPT_GMCP);
    // Request permission to replace IAC GA with IAC EOR
    requestTelnetOption(TN_WILL, OPT_EOR);
}

void UserTelnet::slot_onAnalyzeUserStream(const QByteArray &data)
{
    onReadInternal(data);
}

void UserTelnet::slot_onSendToUser(const QByteArray &ba, const bool goAhead)
{
    // NOTE: We could avoid some overhead by sending one line at a time with a custom ostream.
    auto outdata = encodeForUser(getEncoding(), ba, goAhead);
    submitOverTelnet(outdata, goAhead);
}

void UserTelnet::slot_onGmcpToUser(const GmcpMessage &msg)
{
    if (!myOptionState[OPT_GMCP])
        return;

    const auto name = msg.getName().getStdString();
    const std::size_t found = name.find_last_of('.');
    try {
        const GmcpModule module(name.substr(0, found));
        if (gmcp.modules.find(module) != gmcp.modules.end())
            sendGmcpMessage(msg);

    } catch (const std::exception &e) {
        qWarning() << "Message" << msg.toRawBytes() << "error because:" << e.what();
    }
}

void UserTelnet::virt_sendToMapper(const QByteArray &data, const bool goAhead)
{
    // MMapper requires all data to be Latin-1 internally
    // REVISIT: This will break things if the client is handling MPI itself
    auto indata = decodeFromUser(getEncoding(), data);
    emit sig_analyzeUserStream(indata, goAhead);
}

void UserTelnet::slot_onRelayEchoMode(const bool isDisabled)
{
    sendTelnetOption(isDisabled ? TN_WONT : TN_WILL, OPT_ECHO);
    myOptionState[OPT_ECHO] = !isDisabled;
    announcedState[OPT_ECHO] = true;
}

void UserTelnet::virt_receiveGmcpMessage(const GmcpMessage &msg)
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
        for (const GmcpModule &module : gmcp.modules) {
            // REVISIT: Are some MMapper supported modules not supposed to be filtered?
            if (module.isSupported())
                continue;
            if (comma)
                oss << ", ";
            oss << '"' << module.toStdString() << '"';
            comma = true;
        }
        oss << " ]";
        if (!comma) {
            if (debug)
                qDebug() << "All modules were supported or nothing was requested";
            return;
        }
        GmcpMessage filteredMsg(GmcpMessageTypeEnum::CORE_SUPPORTS_SET, oss.str());
        emit sig_relayGmcp(filteredMsg);
        return;
    }

    emit sig_relayGmcp(msg);
}

void UserTelnet::virt_receiveTerminalType(const QByteArray &data)
{
    if (debug)
        qDebug() << "Received Terminal Type" << data;
    emit sig_relayTermType(data);
}

void UserTelnet::virt_receiveWindowSize(const int x, const int y)
{
    emit sig_relayNaws(x, y);
}

/** Send out the data. Does not double IACs, this must be done
            by caller if needed. This function is suitable for sending
            telnet sequences. */
void UserTelnet::virt_sendRawData(const std::string_view data)
{
    sentBytes += data.length();
    emit sig_sendToSocket(::toQByteArrayLatin1(data));
}

bool UserTelnet::virt_isGmcpModuleEnabled(const GmcpModuleTypeEnum &name)
{
    if (!myOptionState[OPT_GMCP])
        return false;

    return gmcp.supported[name] != DEFAULT_GMCP_MODULE_VERSION;
}

void UserTelnet::receiveGmcpModule(const GmcpModule &module, const bool enabled)
{
    if (enabled) {
        if (!module.hasVersion())
            throw std::runtime_error("missing version");
        gmcp.modules.insert(module);
        if (module.isSupported())
            gmcp.supported[module.getType()] = module.getVersion();

    } else {
        gmcp.modules.erase(module);
        if (module.isSupported())
            gmcp.supported[module.getType()] = DEFAULT_GMCP_MODULE_VERSION;
    }
}

void UserTelnet::resetGmcpModules()
{
    if (debug)
        qDebug() << "Clearing GMCP modules";
#define X_CASE(UPPER_CASE, CamelCase, normalized, friendly) \
    gmcp.supported[GmcpModuleTypeEnum::UPPER_CASE] = DEFAULT_GMCP_MODULE_VERSION;
    X_FOREACH_GMCP_MODULE_TYPE(X_CASE)
#undef X_CASE
    gmcp.modules.clear();
}
