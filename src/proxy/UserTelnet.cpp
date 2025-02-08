// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "UserTelnet.h"

#include "../global/AnsiTextUtils.h"
#include "../global/CharUtils.h"
#include "../global/Charset.h"
#include "../global/Consts.h"
#include "../global/LineUtils.h"
#include "../global/TextUtils.h"
#include "../global/emojis.h"

#include <ostream>
#include <sstream>

// REVISIT: Does this belong somewhere else?
// REVISIT: Should this also normalize ANSI?
static void normalizeForUser(std::ostream &os, const bool goAhead, const std::string_view sv)
{
    // REVISIT: perform ANSI normalization in this function, too?
    foreachLine(sv, [&os, goAhead](std::string_view line) {
        if (line.empty()) {
            return;
        }

        // const bool hasAnsi = containsAnsi(line);
        const bool hasNewline = line.back() == char_consts::C_NEWLINE;
        trim_newline_inplace(line);

        if (!line.empty()) {
            using char_consts::C_CARRIAGE_RETURN;
            foreachCharMulti2(line, C_CARRIAGE_RETURN, [&os, goAhead](std::string_view txt) {
                // so it's only allowed to contain carriage returns if goAhead is true?
                // why not just remove all of the extra carriage returns?
                if (!txt.empty() && (txt.front() != C_CARRIAGE_RETURN || goAhead)) {
                    os << txt;
                }
            });
        }

        if (hasNewline) {
            // REVISIT: add an Ansi reset if the string doesn't contain one?
            // if (hasAnsi) {}
            os << string_consts::SV_CRLF;
        }
    });
}

NODISCARD static QString normalizeForUser(const CharacterEncodingEnum userEncoding,
                                          const QString &s,
                                          const bool goAhead)
{
    auto out = [goAhead, &userEncoding, &s]() -> std::string {
        std::ostringstream oss;
        normalizeForUser(oss,
                         goAhead,
                         mmqt::toStdStringUtf8((getConfig().parser.decodeEmoji
                                                && userEncoding == CharacterEncodingEnum::UTF8
                                                && s.contains(char_consts::C_COLON))
                                                   ? mmqt::decodeEmojiShortCodes(s)
                                                   : s));
        return std::move(oss).str();
    }();

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

UserTelnetOutputs::~UserTelnetOutputs() = default;

UserTelnet::UserTelnet(UserTelnetOutputs &outputs)
    : AbstractTelnet(TextCodecStrategyEnum::AUTO_SELECT_CODEC, TelnetTermTypeBytes{"unknown"})
    , m_outputs{outputs}
{}

void UserTelnet::onConnected()
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

void UserTelnet::onAnalyzeUserStream(const TelnetIacBytes &data)
{
    onReadInternal(data);
}

void UserTelnet::onSendToUser(const QString &s, const bool goAhead)
{
    auto outdata = normalizeForUser(getEncoding(), s, goAhead);
    submitOverTelnet(outdata, goAhead);
}

void UserTelnet::onGmcpToUser(const GmcpMessage &msg)
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

void UserTelnet::onSendMSSPToUser(const TelnetMsspBytes &data)
{
    if (!getOptions().myOptionState[OPT_MSSP]) {
        return;
    }

    sendMudServerStatus(data);
}

void UserTelnet::virt_sendToMapper(const RawBytes &data, const bool goAhead)
{
    m_outputs.onAnalyzeUserStream(decodeFromUser(getEncoding(), data), goAhead);
}

void UserTelnet::onRelayEchoMode(const bool isDisabled)
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

    const bool requiresRewrite = msg.getJson()
                                 && (msg.isCoreSupportsAdd() || msg.isCoreSupportsSet()
                                     || msg.isCoreSupportsRemove())
                                 && msg.getJsonDocument().has_value()
                                 && msg.getJsonDocument()->isArray();

    if (!requiresRewrite) {
        m_outputs.onRelayGmcpFromUserToMud(msg);
        return;
    }

    // Eat Core.Supports.[Add|Set|Remove] and proxy a MMapper filtered subset
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
            qWarning() << "Module" << moduleStr << (msg.isCoreSupportsRemove() ? "remove" : "add")
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

    const GmcpMessage filteredMsg(GmcpMessageTypeEnum::CORE_SUPPORTS_SET,
                                  GmcpJson{std::move(oss).str()});
    m_outputs.onRelayGmcpFromUserToMud(filteredMsg);
}

void UserTelnet::virt_receiveTerminalType(const TelnetTermTypeBytes &data)
{
    if (getDebug()) {
        qDebug() << "Received Terminal Type" << data;
    }
    m_outputs.onRelayTermTypeFromUserToMud(data);
}

void UserTelnet::virt_receiveWindowSize(const int x, const int y)
{
    m_outputs.onRelayNawsFromUserToMud(x, y);
}

void UserTelnet::virt_sendRawData(const TelnetIacBytes &data)
{
    m_outputs.onSendToSocket(data);
}

bool UserTelnet::virt_isGmcpModuleEnabled(const GmcpModuleTypeEnum &name) const
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
