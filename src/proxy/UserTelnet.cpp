// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "UserTelnet.h"

#include "../global/CharUtils.h"
#include "../global/Charset.h"
#include "../global/Consts.h"
#include "../global/LineUtils.h"
#include "../global/TextUtils.h"

#include <ostream>
#include <sstream>

#include <QJsonDocument>

// REVISIT: Does this belong somewhere else?
static void normalizeForUser(std::ostream &os, const bool goAhead, const std::string_view sv)
{
    // REVISIT: perform ANSI normalization in this function, too?
    foreachLine(sv, [&os, &goAhead](std::string_view line) {
        using char_consts::C_CARRIAGE_RETURN;
        using char_consts::C_NEWLINE;
        if (line.empty()) {
            return;
        }

        const bool hasNewline = line.back() == C_NEWLINE;
        if (hasNewline) {
            line.remove_suffix(1);
        }

        if (!line.empty() && line.back() == C_CARRIAGE_RETURN) {
            line.remove_suffix(1);
        }

        if (!line.empty()) {
            foreachCharMulti2(line, C_CARRIAGE_RETURN, [&os, &goAhead](std::string_view txt) {
                // so it's only allowed to contain carriage returns if goAhead is true?
                // why not just remove all of the extra carriage returns?
                if (!txt.empty() && (txt.front() != C_CARRIAGE_RETURN || goAhead)) {
                    os << txt;
                }
            });
        }

        if (hasNewline) {
            os << C_CARRIAGE_RETURN;
            os << C_NEWLINE;
        }
    });
}

NODISCARD static QString encodeForUser(const CharacterEncodingEnum userEncoding,
                                       const QString &s,
                                       const bool goAhead)
{
    // step 1: normalize lines
    auto out = [goAhead, &s]() -> std::string {
        std::ostringstream oss;
        normalizeForUser(oss, goAhead, mmqt::toStdStringUtf8(s));
        return std::move(oss).str();
    }();

    // encoding is done in the base class, so why isn't this done in the base class too?
    if (false) {
        // step 2: convert charset, if necessary
        if (userEncoding != CharacterEncodingEnum::UTF8) {
            std::ostringstream oss;
            charset::conversion::convert(oss, out, CharacterEncodingEnum::UTF8, userEncoding);
            out = std::move(oss).str();
        }
    }

    return mmqt::toQStringUtf8(out);
}

NODISCARD static RawBytes decodeFromUser(const CharacterEncodingEnum userEncoding,
                                         const RawBytes &raw)
{
    if (userEncoding == CharacterEncodingEnum::UTF8) {
        return raw;
    }
    std::ostringstream oss;
    charset::conversion::convert(oss,
                                 mmqt::toStdStringViewRaw(raw.getQByteArray()),
                                 userEncoding,
                                 CharacterEncodingEnum::UTF8);
    return RawBytes{mmqt::toQByteArrayUtf8(oss.str())};
}

UserTelnet::UserTelnet(QObject *const parent)
    : AbstractTelnet(TextCodecStrategyEnum::AUTO_SELECT_CODEC,
                     parent,
                     TelnetTermTypeBytes{"unknown"})
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

void UserTelnet::slot_onAnalyzeUserStream(const TelnetIacBytes &data)
{
    onReadInternal(data);
}

void UserTelnet::slot_onSendToUser(const QString &s, const bool goAhead)
{
    // NOTE: We could avoid some overhead by sending one line at a time with a custom ostream.
    auto outdata = encodeForUser(getEncoding(), s, goAhead);
    submitOverTelnet(outdata, goAhead);
}

void UserTelnet::slot_onGmcpToUser(const GmcpMessage &msg)
{
    if (!getOptions().myOptionState[OPT_GMCP]) {
        return;
    }

    const auto name = msg.getName().getStdStringUtf8();
    const std::size_t found = name.find_last_of(char_consts::C_PERIOD);
    try {
        const GmcpModule mod{name.substr(0, found)};
        if (m_gmcp.modules.find(mod) != m_gmcp.modules.end()) {
            sendGmcpMessage(msg);
        }

    } catch (const std::exception &e) {
        qWarning() << "Message" << msg.toRawBytes() << "error because:" << e.what();
    }
}

void UserTelnet::slot_onSendMSSPToUser(const TelnetMsspBytes &data)
{
    if (!getOptions().myOptionState[OPT_MSSP]) {
        return;
    }

    sendMudServerStatus(data);
}

void UserTelnet::virt_sendToMapper(const RawBytes &data, const bool goAhead)
{
    emit sig_analyzeUserStream(decodeFromUser(getEncoding(), data), goAhead);
}

void UserTelnet::slot_onRelayEchoMode(const bool isDisabled)
{
    sendTelnetOption(isDisabled ? TN_WONT : TN_WILL, OPT_ECHO);

    // REVISIT: This is he only non-const use of the options variable;
    // it could be refactored so the base class does the writes.
    m_options.myOptionState[OPT_ECHO] = !isDisabled;
    m_options.announcedState[OPT_ECHO] = true;
}

void UserTelnet::virt_receiveGmcpMessage(const GmcpMessage &msg)
{
    // Eat Core.Hello since MMapper sends its own to MUME
    if (msg.isCoreHello()) {
        return;
    }

    // Eat Core.Supports.[Add|Set|Remove] and proxy a MMapper filtered subset
    if (msg.getJson()
        && (msg.isCoreSupportsAdd() || msg.isCoreSupportsSet() || msg.isCoreSupportsRemove())
        && msg.getJsonDocument().has_value() && msg.getJsonDocument()->isArray()) {
        // Handle the various messages
        const auto &array = msg.getJsonDocument()->array();
        if (msg.isCoreSupportsSet()) {
            resetGmcpModules();
        }
        for (const auto &e : array) {
            if (!e.isString()) {
                continue;
            }
            const auto &moduleStr = e.toString();
            try {
                const GmcpModule mod{mmqt::toStdStringUtf8(moduleStr)};
                receiveGmcpModule(mod, !msg.isCoreSupportsRemove());

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
        for (const GmcpModule &mod : m_gmcp.modules) {
            // REVISIT: Are some MMapper supported modules not supposed to be filtered?
            if (mod.isSupported()) {
                continue;
            }
            if (comma) {
                oss << ", ";
            }
            oss << char_consts::C_DQUOTE << mod.toStdString() << char_consts::C_DQUOTE;
            comma = true;
        }
        oss << " ]";
        if (!comma) {
            if (getDebug()) {
                qDebug() << "All modules were supported or nothing was requested";
            }
            return;
        }
        GmcpMessage filteredMsg(GmcpMessageTypeEnum::CORE_SUPPORTS_SET,
                                GmcpJson{std::move(oss).str()});
        emit sig_relayGmcp(filteredMsg);
        return;
    }

    emit sig_relayGmcp(msg);
}

void UserTelnet::virt_receiveTerminalType(const TelnetTermTypeBytes &data)
{
    if (getDebug()) {
        qDebug() << "Received Terminal Type" << data;
    }
    emit sig_relayTermType(data);
}

void UserTelnet::virt_receiveWindowSize(const int x, const int y)
{
    emit sig_relayNaws(x, y);
}

/** Send out the data. Does not double IACs, this must be done
            by caller if needed. This function is suitable for sending
            telnet sequences. */
void UserTelnet::virt_sendRawData(const TelnetIacBytes &data)
{
    m_sentBytes += data.length();
    emit sig_sendToSocket(data);
}

bool UserTelnet::virt_isGmcpModuleEnabled(const GmcpModuleTypeEnum &name)
{
    if (!getOptions().myOptionState[OPT_GMCP]) {
        return false;
    }

    return m_gmcp.supported[name] != DEFAULT_GMCP_MODULE_VERSION;
}

void UserTelnet::receiveGmcpModule(const GmcpModule &mod, const bool enabled)
{
    if (enabled) {
        if (!mod.hasVersion()) {
            throw std::runtime_error("missing version");
        }
        m_gmcp.modules.insert(mod);
        if (mod.isSupported()) {
            m_gmcp.supported[mod.getType()] = mod.getVersion();
        }

    } else {
        m_gmcp.modules.erase(mod);
        if (mod.isSupported()) {
            m_gmcp.supported[mod.getType()] = DEFAULT_GMCP_MODULE_VERSION;
        }
    }
}

void UserTelnet::resetGmcpModules()
{
    if (getDebug()) {
        qDebug() << "Clearing GMCP modules";
    }
#define X_CASE(UPPER_CASE, CamelCase, normalized, friendly) \
    m_gmcp.supported[GmcpModuleTypeEnum::UPPER_CASE] = DEFAULT_GMCP_MODULE_VERSION;
    XFOREACH_GMCP_MODULE_TYPE(X_CASE)
#undef X_CASE
    m_gmcp.modules.clear();
}
