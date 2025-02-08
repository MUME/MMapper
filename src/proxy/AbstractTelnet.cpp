// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2002-2005 by Tomas Mecir - kmuddy@kmuddy.com
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "AbstractTelnet.h"

#include "../configuration/configuration.h"
#include "../global/CharUtils.h"
#include "../global/Charset.h"
#include "../global/utils.h"
#include "TextCodec.h"

#ifndef MMAPPER_NO_ZLIB
#include <zlib.h>
#endif

#include <array>
#include <cassert>
#include <functional>
#include <limits>
#include <optional>
#include <ostream>
#include <sstream>
#include <utility>

#include <QByteArray>
#include <QMessageLogContext>
#include <QObject>
#include <QString>

NODISCARD static QString telnetCommandName(uint8_t cmd)
{
#define CASE(x) \
    case TN_##x: \
        return #x
    switch (cmd) {
        CASE(SE);
        CASE(NOP);
        CASE(DM);
        CASE(B);
        CASE(IP);
        CASE(AO);
        CASE(AYT);
        CASE(EC);
        CASE(EL);
        CASE(GA);
        CASE(SB);
        CASE(WILL);
        CASE(WONT);
        CASE(DO);
        CASE(DONT);
        CASE(IAC);
        CASE(EOR);
    }
    return QString::asprintf("%u", cmd);
#undef CASE
}

NODISCARD static QString telnetOptionName(uint8_t opt)
{
#define CASE(x) \
    case OPT_##x: \
        return #x
    switch (opt) {
        CASE(ECHO);
        CASE(SUPPRESS_GA);
        CASE(STATUS);
        CASE(TIMING_MARK);
        CASE(TERMINAL_TYPE);
        CASE(NAWS);
        CASE(CHARSET);
        CASE(COMPRESS2);
        CASE(GMCP);
        CASE(MSSP);
        CASE(LINEMODE);
        CASE(EOR);
    }
    return QString::asprintf("%u", opt);
#undef CASE
}
NODISCARD static QString telnetSubnegName(uint8_t opt)
{
#define CASE(x) \
    case TNSB_##x: \
        return #x
    switch (opt) {
        CASE(IS);
        CASE(SEND); // TODO: conflict between SEND, REQUEST, EDIT, MODE
        CASE(ACCEPTED);
        CASE(REJECTED);
        CASE(TTABLE_IS);
        CASE(TTABLE_REJECTED);
        CASE(TTABLE_ACK);
        CASE(TTABLE_NAK);
    }
    return QString::asprintf("%u", opt);
#undef CASE
}

NODISCARD static bool containsIAC(const std::string_view arr)
{
    return arr.find(char(TN_IAC)) != std::string_view::npos;
}

NODISCARD static bool containsIAC(const RawBytes &raw)
{
    return containsIAC(mmqt::toStdStringViewRaw(raw.getQByteArray()));
}

static void doubleIacs(std::ostream &os, const std::string_view input)
{
    // IAC byte must be doubled
    static constexpr const auto IAC = static_cast<char>(TN_IAC);

    // Note: input isn't required to be latin-1, but we're treating it as-if it is
    // because the only thing that matters is the 255 byte.
    foreachLatin1CharSingle(
        input,
        IAC,
        [&os]() { os << IAC << IAC; },
        [&os](const std::string_view sv) {
            if (!sv.empty()) {
                os << sv;
            }
        });
}

NODISCARD static TelnetIacBytes doubleIacs(const RawBytes &raw)
{
    if (!containsIAC(raw)) {
        return TelnetIacBytes{raw.getQByteArray()};
    }

    const auto sv = mmqt::toStdStringViewRaw(raw.getQByteArray());
    std::ostringstream os;
    doubleIacs(os, sv);
    return TelnetIacBytes{mmqt::toQByteArrayRaw(os.str())};
}

namespace mmqt {
NODISCARD static std::string toEncoding(const QString &s, const CharacterEncodingEnum encoding)
{
    switch (encoding) {
    case CharacterEncodingEnum::UTF8:
        return mmqt::toStdStringUtf8(s);
    case CharacterEncodingEnum::LATIN1:
        return mmqt::toStdStringLatin1(s); // conversion to user charset
    case CharacterEncodingEnum::ASCII: {
        auto ascii = mmqt::toStdStringLatin1(s);          // conversion to user charset
        charset::conversion::latin1ToAsciiInPlace(ascii); // conversion to user charset
        return ascii;
    }
    default:
        abort();
    }
}

NODISCARD static RawBytes toRawBytes(const QString &qs, const CharacterEncodingEnum encoding)
{
    const auto s = toEncoding(qs, encoding);
    return RawBytes{mmqt::toQByteArrayRaw(s)};
}

} // namespace mmqt

// Emits output via AbstractTelnet::sendRawData() as RAII callback in dtor.
struct NODISCARD TelnetFormatter final
{
private:
    AbstractTelnet &m_telnet;
    std::ostringstream m_os;

public:
    explicit TelnetFormatter(AbstractTelnet &telnet)
        : m_telnet{telnet}
    {}
    ~TelnetFormatter()
    {
        try {
            auto str = m_os.str();
            m_telnet.sendRawData(TelnetIacBytes{mmqt::toQByteArrayRaw(str)});
        } catch (const std::exception &ex) {
            qWarning() << "Exception while sending raw data:" << ex.what();
        } catch (...) {
            qWarning() << "Unknown exception while sending raw data.";
        }
    }

    DELETE_CTORS_AND_ASSIGN_OPS(TelnetFormatter);

public:
    void addRaw(const uint8_t byte) { m_os << static_cast<char>(byte); }

    void addEscaped(const uint8_t byte)
    {
        addRaw(byte);
        if (byte == TN_IAC) {
            addRaw(byte);
        }
    }

    void addTwoByteEscaped(const uint16_t n)
    {
        // network order is big-endian
        addEscaped(static_cast<uint8_t>(n >> 8));
        addEscaped(static_cast<uint8_t>(n));
    }

    void addClampedTwoByteEscaped(const int n)
    {
        static constexpr const auto lo = static_cast<int>(std::numeric_limits<uint16_t>::min());
        static constexpr const auto hi = static_cast<int>(std::numeric_limits<uint16_t>::max());
        static_assert(lo == 0);
        static_assert(hi == 65535);
        addTwoByteEscaped(static_cast<uint16_t>(std::clamp(n, lo, hi)));
    }

    void addEscapedBytes(const QByteArray &s)
    {
        for (const auto &c : s) {
            addEscaped(static_cast<uint8_t>(c));
        }
    }

    void addCommand(uint8_t cmd)
    {
        addRaw(TN_IAC);
        addRaw(cmd);
    }

    void addSubnegBegin(uint8_t opt)
    {
        addCommand(TN_SB);
        addRaw(opt);
    }

    void addSubnegEnd() { addCommand(TN_SE); }
};

struct NODISCARD AbstractTelnet::ZstreamPimpl final
{
#ifndef MMAPPER_NO_ZLIB
    z_stream zstream{};
    explicit ZstreamPimpl();
    ~ZstreamPimpl();
#endif
    void reset();
};

#ifndef MMAPPER_NO_ZLIB
AbstractTelnet::ZstreamPimpl::ZstreamPimpl()
{
    auto &stream = zstream;
    /* allocate inflate state */
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;
    stream.avail_in = 0;
    stream.next_in = Z_NULL;
    const int ret = inflateInit(&stream);
    if (ret != Z_OK) {
        throw std::runtime_error("Unable to initialize zlib");
    }
}

AbstractTelnet::ZstreamPimpl::~ZstreamPimpl()
{
    inflateEnd(&zstream);
}
#endif

void AbstractTelnet::ZstreamPimpl::reset()
{
#ifndef MMAPPER_NO_ZLIB
    const int ret = inflateReset(&zstream);
    if (ret != Z_OK) {
        throw std::runtime_error("Could not reset zlib");
    }
#endif
}

AbstractTelnet::AbstractTelnet(const TextCodecStrategyEnum strategy,
                               TelnetTermTypeBytes defaultTermType)
    : m_defaultTermType(std::move(defaultTermType))
    , m_textCodec{strategy}
    , m_zstream_pimpl{std::make_unique<ZstreamPimpl>()}
{
    reset();
}

AbstractTelnet::~AbstractTelnet() = default;

void AbstractTelnet::Options::reset()
{
    // Could also use "*this = {};" instead of the following:
    myOptionState.fill(false);
    hisOptionState.fill(false);
    announcedState.fill(false);
    heAnnouncedState.fill(false);
    triedToEnable.fill(false);
}

void AbstractTelnet::reset()
{
    if (m_debug) {
        qDebug() << "Reset telnet";
    }

    m_options.reset();

    // reset telnet status
    m_termType = m_defaultTermType;
    m_state = TelnetStateEnum::NORMAL;
    m_commandBuffer.clear();
    m_subnegBuffer.clear();
    m_sentBytes = 0;
    m_recvdGA = false;

    assert(m_options.hisOptionState[OPT_COMPRESS2] == false);
    resetCompress();
}

void AbstractTelnet::submitOverTelnet(const QString &s, const bool goAhead)
{
    const auto rawBytes = mmqt::toRawBytes(s, getEncoding());
    submitOverTelnet(rawBytes, goAhead);
}

void AbstractTelnet::trySendGoAhead()
{
    const auto &myOptionState = m_options.myOptionState;
    const auto hasEor = myOptionState[OPT_EOR];
    if (myOptionState[OPT_SUPPRESS_GA] && !hasEor) {
        return;
    }

    const uint8_t buf[2]{TN_IAC, hasEor ? TN_EOR : TN_GA};
    const auto ba = QByteArray{reinterpret_cast<const char *>(buf), 2};
    sendRawData(TelnetIacBytes{ba});
}

void AbstractTelnet::sendWithDoubledIacs(const RawBytes &raw)
{
    sendRawData(doubleIacs(raw));
}

void AbstractTelnet::submitOverTelnet(const RawBytes &data, const bool goAhead)
{
    sendWithDoubledIacs(data);
    if (goAhead) {
        trySendGoAhead();
    }
}

void AbstractTelnet::sendWindowSizeChanged(const int width, const int height)
{
    if (m_debug) {
        qDebug() << "Sending NAWS" << width << height;
    }

    // RFC 1073 specifies IAC SB NAWS WIDTH[1] WIDTH[0] HEIGHT[1] HEIGHT[0] IAC SE
    TelnetFormatter s{*this};
    s.addSubnegBegin(OPT_NAWS);
    // RFC855 specifies that option parameters with a byte value of 255 must be doubled
    s.addClampedTwoByteEscaped(width);
    s.addClampedTwoByteEscaped(height);
    s.addSubnegEnd();
}

void AbstractTelnet::sendTelnetOption(unsigned char type, unsigned char option)
{
    // Do not respond if we initiated this request
    auto &triedToEnable = m_options.triedToEnable;
    if (triedToEnable[option]) {
        triedToEnable[option] = false;
        return;
    }

    if (m_debug) {
        qDebug() << "* Sending Telnet Command: " << telnetCommandName(type)
                 << telnetOptionName(option);
    }

    TelnetFormatter s{*this};
    s.addRaw(TN_IAC);
    s.addRaw(type);
    s.addRaw(option);
}

void AbstractTelnet::requestTelnetOption(unsigned char type, unsigned char option)
{
    // Set his option state correctly
    if (type == TN_DO || type == TN_DONT) {
        m_options.hisOptionState[option] = (type == TN_DO);
    }

    sendTelnetOption(type, option);

    m_options.triedToEnable[option] = true;
}

void AbstractTelnet::sendCharsetRequest()
{
    // REVISIT: RFC 2066 states to queue all subsequent data until ACCEPTED / REJECTED

    QStringList myCharacterSets;
    for (const auto &encoding : m_textCodec.supportedEncodings()) {
        myCharacterSets << mmqt::toQByteArrayUtf8(encoding);
    }
    if (m_debug) {
        qDebug() << "Sending Charset request" << myCharacterSets;
    }

    static constexpr const auto delimeter = ";";

    TelnetFormatter s{*this};
    s.addSubnegBegin(OPT_CHARSET);
    s.addRaw(TNSB_REQUEST);
    for (const auto &characterSet : myCharacterSets) {
        s.addEscapedBytes(delimeter);
        s.addEscapedBytes(characterSet.toUtf8());
    }
    s.addSubnegEnd();
}

void AbstractTelnet::sendGmcpMessage(const GmcpMessage &msg)
{
    auto payload = msg.toRawBytes();
    if (m_debug) {
        qDebug() << "Sending GMCP:" << payload;
    }

    TelnetFormatter s{*this};
    s.addSubnegBegin(OPT_GMCP);
    s.addEscapedBytes(payload);
    s.addSubnegEnd();
}

void AbstractTelnet::sendMudServerStatus(const TelnetMsspBytes &data)
{
    if (m_debug) {
        qDebug() << "Sending MSSP:" << data;
    }

    TelnetFormatter s{*this};
    s.addSubnegBegin(OPT_MSSP);
    s.addEscapedBytes(data.getQByteArray());
    s.addSubnegEnd();
}

void AbstractTelnet::sendLineModeEdit()
{
    if (m_debug) {
        qDebug() << "Sending Linemode EDIT";
    }

    TelnetFormatter s{*this};
    s.addSubnegBegin(OPT_LINEMODE);
    s.addRaw(TNSB_MODE);
    s.addRaw(TNSB_EDIT);
    s.addSubnegEnd();
}

void AbstractTelnet::sendTerminalType(const TelnetTermTypeBytes &terminalType)
{
    if (m_debug) {
        qDebug() << "Sending Terminal Type:" << terminalType;
    }

    TelnetFormatter s{*this};
    s.addSubnegBegin(OPT_TERMINAL_TYPE);
    // RFC855 specifies that option parameters with a byte value of 255 must be doubled
    s.addEscaped(TNSB_IS); /* NOTE: "IS" will never actually be escaped */
    s.addEscapedBytes(terminalType.getQByteArray());
    s.addSubnegEnd();
}

void AbstractTelnet::sendCharsetRejected()
{
    TelnetFormatter s{*this};
    s.addSubnegBegin(OPT_CHARSET);
    s.addRaw(TNSB_REJECTED);
    s.addSubnegEnd();
}

void AbstractTelnet::sendCharsetAccepted(const TelnetCharsetBytes &characterSet)
{
    if (m_debug) {
        qDebug() << "Accepted Charset" << characterSet;
    }

    TelnetFormatter s{*this};
    s.addSubnegBegin(OPT_CHARSET);
    s.addRaw(TNSB_ACCEPTED);
    s.addEscapedBytes(characterSet.getQByteArray());
    s.addSubnegEnd();
}

void AbstractTelnet::sendOptionStatus()
{
    TelnetFormatter s{*this};
    s.addRaw(TN_IAC);
    s.addRaw(TN_SB);
    s.addRaw(OPT_STATUS);
    s.addRaw(TNSB_IS);
    for (size_t i = 0; i < NUM_OPTS; ++i) {
        if (m_options.myOptionState[i]) {
            s.addRaw(TN_WILL);
            s.addRaw(static_cast<unsigned char>(i));
        }
        if (m_options.hisOptionState[i]) {
            s.addRaw(TN_DO);
            s.addRaw(static_cast<unsigned char>(i));
        }
    }
    s.addRaw(TN_IAC);
    s.addRaw(TN_SE);
}

void AbstractTelnet::sendTerminalTypeRequest()
{
    if (m_debug) {
        qDebug() << "Requesting Terminal Type";
    }
    TelnetFormatter s{*this};
    s.addSubnegBegin(OPT_TERMINAL_TYPE);
    s.addEscaped(TNSB_SEND);
    s.addSubnegEnd();
}

void AbstractTelnet::processTelnetCommand(const AppendBuffer &command)
{
    assert(!command.isEmpty());

    auto &myOptionState = m_options.myOptionState;
    auto &hisOptionState = m_options.hisOptionState;
    auto &announcedState = m_options.announcedState;
    auto &heAnnouncedState = m_options.heAnnouncedState;

    const unsigned char ch = command.unsigned_at(1);
    unsigned char option;

    switch (command.length()) {
    case 2:
        if (m_debug && ch != TN_GA && ch != TN_EOR) {
            qDebug() << "* Processing Telnet Command:" << telnetCommandName(ch);
        }

        switch (ch) {
        case TN_GA:
        case TN_EOR:
            m_recvdGA = true; // signal will be emitted later
            break;
        }
        break;

    case 3:
        option = command.unsigned_at(2);

        if (m_debug) {
            qDebug() << "* Processing Telnet Command:" << telnetCommandName(ch)
                     << telnetOptionName(option);
        }

        switch (ch) {
        case TN_WILL:
            // peer wants to enable some option (or he sends a timing-mark)...
            if (!hisOptionState[option] || !heAnnouncedState[option]) {
                // only if the option is currently disabled
                // these options are supported
                if ((option == OPT_SUPPRESS_GA) || (option == OPT_STATUS)
                    || (option == OPT_TERMINAL_TYPE) || (option == OPT_NAWS) || (option == OPT_ECHO)
                    || (option == OPT_CHARSET) || (option == OPT_COMPRESS2 && !NO_ZLIB)
                    || (option == OPT_GMCP) || (option == OPT_MSSP) || (option == OPT_LINEMODE)
                    || (option == OPT_EOR)) {
                    sendTelnetOption(TN_DO, option);
                    hisOptionState[option] = true;

                    if (option == OPT_ECHO) {
                        receiveEchoMode(false);
                    } else if (option == OPT_LINEMODE) {
                        sendLineModeEdit();
                    } else if (option == OPT_GMCP) {
                        onGmcpEnabled();
                    } else if (option == OPT_TERMINAL_TYPE) {
                        sendTerminalTypeRequest();
                    } else if (option == OPT_CHARSET) {
                        sendCharsetRequest();
                    }
                } else {
                    sendTelnetOption(TN_DONT, option);
                    hisOptionState[option] = false;
                }
            }
            heAnnouncedState[option] = true;
            break;
        case TN_WONT:
            // peer refuses to enable some option...
            if (hisOptionState[option] || !heAnnouncedState[option]) {
                // send DONT if needed (see RFC 854 for details)
                sendTelnetOption(TN_DONT, option);
                heAnnouncedState[option] = true;
            }
            hisOptionState[option] = false;
            if (option == OPT_ECHO) {
                receiveEchoMode(true);
            }
            break;
        case TN_DO:
            // peer allows us to enable some option
            if (option == OPT_TIMING_MARK) {
                // send WILL TIMING_MARK
                sendTelnetOption(TN_WILL, option);
                break;
            }

            // Ignore attempts to enable OPT_ECHO
            if (option == OPT_ECHO) {
                break;
            }

            // only respond if value changed or if this option has not been announced yet
            if (!myOptionState[option] || !announcedState[option]) {
                // only if the option is currently disabled
                if ((option == OPT_SUPPRESS_GA) || (option == OPT_STATUS)
                    || (option == OPT_TERMINAL_TYPE) || (option == OPT_NAWS)
                    || (option == OPT_CHARSET) || (option == OPT_GMCP) || (option == OPT_LINEMODE)
                    || (option == OPT_EOR)) {
                    sendTelnetOption(TN_WILL, option);
                    myOptionState[option] = true;
                    if (option == OPT_NAWS) {
                        // NAWS here - window size info must be sent
                        // REVISIT: Should we attempt to rate-limit this to avoid spamming dozens of NAWS
                        // messages per second when the user adjusts the window size?
                        sendWindowSizeChanged(m_currentNaws.width, m_currentNaws.height);
                    } else if (option == OPT_GMCP) {
                        onGmcpEnabled();
                    } else if (option == OPT_LINEMODE) {
                        sendLineModeEdit();
                    } else if (option == OPT_COMPRESS2 && !NO_ZLIB) {
                        // REVISIT: Start deflating after sending IAC SB COMPRESS2 IAC SE
                    } else if (option == OPT_CHARSET && heAnnouncedState[option]) {
                        sendCharsetRequest();
                    }
                } else {
                    sendTelnetOption(TN_WONT, option);
                    myOptionState[option] = false;
                }
                announcedState[option] = true;
            }
            break;
        case TN_DONT:
            // only respond if value changed or if this option has not been announced yet
            if (myOptionState[option] || !announcedState[option]) {
                sendTelnetOption(TN_WONT, option);
                announcedState[option] = true;
            }
            myOptionState[option] = false;
            break;
        }
        break;

    default:
        // other cmds should not arrive, as they were not negotiated.
        // if they do, they are merely ignored
        break;
    }
    // other commands are simply ignored (NOP and such, see .h file for list)
}

void AbstractTelnet::processTelnetSubnegotiation(const AppendBuffer &payload)
{
    auto &myOptionState = m_options.myOptionState;
    auto &hisOptionState = m_options.hisOptionState;

    if (m_debug) {
        if (payload.length() == 1) {
            qDebug() << "* Processing Telnet Subnegotiation:"
                     << telnetOptionName(payload.unsigned_at(0));
        } else if (payload.length() >= 2) {
            qDebug() << "* Processing Telnet Subnegotiation:"
                     << telnetOptionName(payload.unsigned_at(0))
                     << telnetSubnegName(payload.unsigned_at(1));
        }
    }

    // subnegotiation - we analyze and respond...
    const auto option = static_cast<uint8_t>(payload[0]);
    switch (option) {
    case OPT_STATUS:
        // see OPT_TERMINAL_TYPE for explanation why I'm doing this
        if (true /*myOptionState[OPT_STATUS]*/) {
            // request to send all enabled commands; if server sends his
            // own list of commands, we just ignore it (well, he shouldn't
            // send anything, as we do not request anything, but there are
            // so many servers out there, that you can never be sure...)
            if (payload[1] == TNSB_SEND) {
                sendOptionStatus();
            }
        }
        break;

    case OPT_TERMINAL_TYPE:
        if (myOptionState[OPT_TERMINAL_TYPE] || hisOptionState[OPT_TERMINAL_TYPE]) {
            switch (payload[1]) {
            case TNSB_SEND:
                // server wants us to send terminal type
                sendTerminalType(m_termType);
                break;
            case TNSB_IS:
                // Extract sender's terminal type
                // TERMINAL_TYPE IS <...>
                receiveTerminalType(TelnetTermTypeBytes{payload.getQByteArray().mid(2)});
                break;
            }
        }
        break;

    case OPT_CHARSET:
        if (hisOptionState[OPT_CHARSET] || myOptionState[OPT_CHARSET]) {
            switch (payload[1]) {
            case TNSB_REQUEST:
                // MMapper does not support [TTABLE]
                if (payload.length() >= 4 && payload[2] != char_consts::C_OPEN_BRACKET) {
                    bool accepted = false;
                    // Split remainder into delim and IAC
                    // CHARSET REQUEST <sep> <charsets>
                    const auto sep = payload[2];
                    const auto characterSets = payload.getQByteArray().mid(3).split(sep);
                    if (m_debug) {
                        qDebug() << "Received encoding options" << characterSets;
                    }
                    for (const auto &characterSet : characterSets) {
                        // expected to be ASCII
                        const auto name = mmqt::toStdStringUtf8(characterSet.simplified());
                        if (m_textCodec.supports(name)) {
                            accepted = true;
                            m_textCodec.setEncodingForName(name);

                            // Reply to server that we accepted this encoding
                            sendCharsetAccepted(TelnetCharsetBytes{characterSet});
                            break;
                        }
                    }
                    if (accepted) {
                        break;
                    }
                }
                // Reject invalid requests or if we did not find any supported codecs
                if (m_debug) {
                    qDebug() << "Rejected all encodings";
                }
                sendCharsetRejected();
                break;
            case TNSB_ACCEPTED:
                if (payload.length() > 3) {
                    // CHARSET ACCEPTED <charset>
                    const auto characterSet = payload.getQByteArray().mid(2).simplified();
                    // expected to be ASCII
                    m_textCodec.setEncodingForName(mmqt::toStdStringUtf8(characterSet));
                    if (m_debug) {
                        qDebug() << "He accepted charset" << characterSet;
                    }
                    // TODO: RFC 2066 states to stop queueing data
                }
                break;
            case TNSB_REJECTED:
                if (m_debug) {
                    qDebug() << "He rejected charset";
                }
                // TODO: RFC 2066 states to stop queueing data
                break;
            case TNSB_TTABLE_IS:
                // We never request a [TTABLE] so this should not happen
                abort();
                /*NOTREACHED*/
                break;
            }
        }
        break;

    case OPT_COMPRESS2:
        assert(!NO_ZLIB);
        if (hisOptionState[OPT_COMPRESS2]) {
            if (m_inflateTelnet) {
                qWarning() << "Compression was already enabled";
                break;
            }
            if (m_debug) {
                qDebug() << "Starting compression";
            }
            m_recvdCompress = true;
        }
        break;

    case OPT_GMCP:
        if (hisOptionState[OPT_GMCP] || myOptionState[OPT_GMCP]) {
            // Package[.SubPackages].Message <data>
            if (payload.length() <= 1) {
                qWarning() << "Invalid GMCP received" << payload;
                break;
            }
            try {
                GmcpMessage msg = GmcpMessage::fromRawBytes(payload.getQByteArray().mid(1));
                if (m_debug) {
                    qDebug() << "Received GMCP message" << msg.getName().toQString()
                             << (msg.getJson() ? msg.getJson()->toQString() : "");
                }
                receiveGmcpMessage(msg);

            } catch (const std::exception &e) {
                qWarning() << "Corrupted GMCP received" << payload << e.what();
            }
        } else {
            qWarning() << "His GMCP is not enabled yet!";
        }
        break;

    case OPT_MSSP:
        if (hisOptionState[OPT_MSSP]) {
            if (m_debug) {
                qDebug() << "Received MSSP message" << payload;
            }

            receiveMudServerStatus(TelnetMsspBytes{payload.getQByteArray()});
        }
        break;

    case OPT_NAWS:
        if (myOptionState[OPT_NAWS] || hisOptionState[OPT_NAWS]) {
            // NAWS <16-bit value> <16-bit value>
            if (payload.length() == 5) {
                const auto x1 = static_cast<uint8_t>(payload[1]);
                const auto x2 = static_cast<uint8_t>(payload[2]);
                const auto y1 = static_cast<uint8_t>(payload[3]);
                const auto y2 = static_cast<uint8_t>(payload[4]);
                const auto x = static_cast<uint16_t>((x1 << 8) + x2);
                const auto y = static_cast<uint16_t>((y1 << 8) + y2);
                receiveWindowSize(x, y);
                break;
            }
            qWarning() << "Corrupted NAWS received" << payload;
        }
        break;

    default:
        // other subnegs should not arrive and if they do, they are merely ignored
        break;
    }
}

void AbstractTelnet::onReadInternal(const TelnetIacBytes &data)
{
    if (data.isEmpty()) {
        return;
    }

    // now we have the data, but we cannot forward it to next stage of processing,
    // because the data contains telnet commands
    // so we parse the text and process all telnet commands:

    AppendBuffer cleanData;
    cleanData.reserve(data.size());

    int pos = 0;
    while (pos < data.size()) {
        if (m_inflateTelnet) {
            const int remaining = onReadInternalInflate(data.getQByteArray().data() + pos,
                                                        data.size() - pos,
                                                        cleanData);
            pos = data.length() - remaining;
            // Continue because there might be additional chunks left to inflate
            continue;
        }

        // Process character by character
        const auto c = static_cast<uint8_t>(data.at(pos));
        ++pos;
        onReadInternal2(cleanData, c);

        if (m_recvdCompress) {
            m_inflateTelnet = true;
            m_recvdCompress = false;
            deref(m_zstream_pimpl).reset();

            // Start inflating at the next position
            continue;
        }

        // Should this be before or after processing m_recvdCompress?
        if (m_recvdGA) {
            processGA(cleanData);
        }
    }

    // some data left to send - do it now!
    if (!cleanData.isEmpty()) {
        sendToMapper(cleanData, m_recvdGA); // without GO-AHEAD
        cleanData.clear();
    }
}

/*
 * normal telnet state
 * -------------------
 * x                                # forward 0-254
 * IAC IAC                          # forward 255
 * IAC (WILL | WONT | DO | DONT) x  # negotiate 0-255 (w/ 255 = EXOPL)
 * IAC SB                           # begins subnegotiation
 * IAC SE                           # (error)
 * IAC x                            # exec command
 *
 * within a subnegotiation
 * -----------------------
 * x                                # appends 0-254 to option payload
 * IAC IAC                          # appends 255 to option payload
 * IAC (WILL | WONT | DO | DONT) x  # negotiate 0-255 (w/ 255 = EXOPL)
 * IAC SB                           # (error)
 * IAC SE                           # ends subnegotiation
 * IAC x                            # exec command
 *
 * NOTE: RFC 855 refers to IAC SE as a command rather than a delimiter,
 * so that implies you're still supposed to process "commands" (e.g. IAC GA)!
 *
 * So if you receive "IAC SB IAC WILL ECHO f o o IAC IAC b a r IAC SE"
 * then you process will(ECHO) followed by the subnegotiation(f o o 255 b a r).
 */
void AbstractTelnet::onReadInternal2(AppendBuffer &cleanData, const uint8_t c)
{
    auto &state = m_state;
    auto &commandBuffer = m_commandBuffer;
    auto &subnegBuffer = m_subnegBuffer;
    switch (state) {
    case TelnetStateEnum::NORMAL:
        if (c == TN_IAC) {
            // this is IAC, previous character was regular data
            state = TelnetStateEnum::IAC;
            commandBuffer.append(c);
        } else {
            // plaintext
            cleanData.append(c);
        }
        break;
    case TelnetStateEnum::IAC:
        // seq. of two IACs
        if (c == TN_IAC) {
            state = TelnetStateEnum::NORMAL;
            cleanData.append(c);
            commandBuffer.clear();
        }
        // IAC DO/DONT/WILL/WONT
        else if ((c == TN_WILL) || (c == TN_WONT) || (c == TN_DO) || (c == TN_DONT)) {
            state = TelnetStateEnum::COMMAND;
            commandBuffer.append(c);
        }
        // IAC SB
        else if (c == TN_SB) {
            state = TelnetStateEnum::SUBNEG;
            commandBuffer.clear();
        }
        // IAC SE without IAC SB - error - ignored
        else if (c == TN_SE) {
            state = TelnetStateEnum::NORMAL;
            commandBuffer.clear();
        }
        // IAC fol. by something else than IAC, SB, SE, DO, DONT, WILL, WONT
        else {
            state = TelnetStateEnum::NORMAL;
            commandBuffer.append(c);
            processTelnetCommand(commandBuffer);
            // this could have set receivedGA to true; we'll handle that later
            // (at the end of this function)
            commandBuffer.clear();
        }
        break;
    case TelnetStateEnum::COMMAND:
        // IAC DO/DONT/WILL/WONT <command code>
        state = TelnetStateEnum::NORMAL;
        commandBuffer.append(c);
        processTelnetCommand(commandBuffer);
        commandBuffer.clear();
        break;

    case TelnetStateEnum::SUBNEG:
        if (c == TN_IAC) {
            // this is IAC, previous character was option payload
            state = TelnetStateEnum::SUBNEG_IAC;
            commandBuffer.append(c);
        } else {
            // option payload
            subnegBuffer.append(c);
        }
        break;
    case TelnetStateEnum::SUBNEG_IAC:
        // seq. of two IACs
        if (c == TN_IAC) {
            state = TelnetStateEnum::SUBNEG;
            subnegBuffer.append(c);
            commandBuffer.clear();
        }
        // IAC DO/DONT/WILL/WONT
        else if ((c == TN_WILL) || (c == TN_WONT) || (c == TN_DO) || (c == TN_DONT)) {
            state = TelnetStateEnum::SUBNEG_COMMAND;
            commandBuffer.append(c);
        }
        // IAC SE - end of subcommand
        else if (c == TN_SE) {
            state = TelnetStateEnum::NORMAL;
            processTelnetSubnegotiation(subnegBuffer);
            commandBuffer.clear();
            subnegBuffer.clear();
        }
        // IAC SB within IAC SB - error - ignored
        else if (c == TN_SB) {
            state = TelnetStateEnum::NORMAL;
            commandBuffer.clear();
            subnegBuffer.clear();
        }
        // IAC fol. by something else than IAC, SB, SE, DO, DONT, WILL, WONT
        else {
            state = TelnetStateEnum::SUBNEG;
            commandBuffer.append(c);
            processTelnetCommand(commandBuffer);
            // this could have set receivedGA to true; we'll handle that later
            // (at the end of this function)
            commandBuffer.clear();
        }
        break;
    case TelnetStateEnum::SUBNEG_COMMAND:
        // IAC DO/DONT/WILL/WONT <command code>
        state = TelnetStateEnum::SUBNEG;
        commandBuffer.append(c);
        processTelnetCommand(commandBuffer);
        commandBuffer.clear();
        break;
    }
}

int AbstractTelnet::onReadInternalInflate(const char *const data,
                                          const int length,
                                          AppendBuffer &cleanData)
{
#ifdef MMAPPER_NO_ZLIB
    abort();
#else
    static constexpr const int CHUNK = 1024;
    alignas(64) char out[CHUNK];

    auto &stream = deref(m_zstream_pimpl).zstream;
    stream.avail_in = static_cast<uint32_t>(length);
    stream.next_in = reinterpret_cast<const Bytef *>(data);

    /* decompress until deflate stream ends */
    do {
        stream.avail_out = CHUNK;
        stream.next_out = reinterpret_cast<Bytef *>(out);
        int ret = inflate(&stream, Z_SYNC_FLUSH);
        assert(ret != Z_STREAM_ERROR); /* state not clobbered */
        if (ret == Z_DATA_ERROR) {
            ret = inflateSync(&stream);
        }
        switch (ret) {
        case Z_NEED_DICT:
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
            throw std::runtime_error(stream.msg);
        case Z_STREAM_END:
            /* clean up and return */
            m_inflateTelnet = false;
            if (m_debug) {
                qDebug() << "Ending compression";
            }
            break;
        default:
            break;
        }

        const int outLen = CHUNK - static_cast<int>(stream.avail_out);
        for (auto i = 0; i < outLen; ++i) {
            // Process character by character
            const auto c = static_cast<uint8_t>(out[i]);
            onReadInternal2(cleanData, c);
            if (m_recvdGA) {
                processGA(cleanData);
            }
        }

    } while (stream.avail_out == 0);
    return static_cast<int>(stream.avail_in);
#endif
}

void AbstractTelnet::resetCompress()
{
    m_inflateTelnet = false;
    m_recvdCompress = false;

    // Should this be called here?
    if ((false)) {
        deref(m_zstream_pimpl).reset();
    }
}

void AbstractTelnet::processGA(AppendBuffer &cleanData)
{
    if (!m_recvdGA) {
        return;
    }

    sendToMapper(cleanData, m_recvdGA); // with GO-AHEAD
    cleanData.clear();
    m_recvdGA = false;
}
