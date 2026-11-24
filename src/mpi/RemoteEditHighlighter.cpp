// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "RemoteEditHighlighter.h"

#include "../global/AnsiTextUtils.h"
#include "../global/CharUtils.h"
#include "../global/Charset.h"
#include "../global/Consts.h"
#include "../global/TextUtils.h"
#include "../global/entities.h"
#include "../global/utils.h"

#include <optional>

#include <QBrush>
#include <QTextDocument>

using namespace char_consts;

namespace {
constexpr QColor darkOrangeQColor{0xFF, 0x8C, 0x00};
constexpr bool USE_TOOLTIPS = false;
} // namespace

RemoteEditHighlighter::RemoteEditHighlighter(QTextDocument *const document,
                                             const int maxLength,
                                             const MumeEditOptions &options)
    : QSyntaxHighlighter(document)
    , m_maxLength{maxLength}
    , m_options{options}
{}

RemoteEditHighlighter::~RemoteEditHighlighter() = default;

QTextCharFormat RemoteEditHighlighter::getBackgroundFormat(const Qt::GlobalColor color)
{
    QTextCharFormat fmt;
    fmt.setBackground(QBrush{color});
    return fmt;
}

QTextCharFormat RemoteEditHighlighter::getBackgroundFormat(const QColor &color)
{
    QTextCharFormat fmt;
    fmt.setBackground(QBrush{color});
    return fmt;
}

void RemoteEditHighlighter::highlightTabs(const QString &line)
{
    if (line.indexOf(C_TAB) < 0) {
        return;
    }

    const QTextCharFormat fmt = getBackgroundFormat(Qt::yellow);
    // REVISIT: Should this be in TabUtils?
    mmqt::foreachCharPos(line, C_TAB, [this, &fmt](const auto at) {
        setFormat(static_cast<int>(at), 1, fmt);
    });
}

void RemoteEditHighlighter::highlightOverflow(const QString &line)
{
    const int breakPos = (mmqt::measureTabAndAnsiAware(line) <= m_maxLength) ? -1 : m_maxLength;
    if (breakPos < 0) {
        return;
    }

    const auto getFmt = []() -> QTextCharFormat {
        QTextCharFormat fmt;
        fmt.setFontUnderline(true);
        fmt.setUnderlineStyle(QTextCharFormat::UnderlineStyle::WaveUnderline);
        fmt.setUnderlineColor(Qt::red);
        return fmt;
    };

    const int length = static_cast<int>(line.length()) - breakPos;
    setFormat(breakPos, length, getFmt());
}

void RemoteEditHighlighter::highlightTrailingSpace(const QString &line)
{
    const int breakPos = mmqt::findTrailingWhitespace(line);
    if (breakPos < 0) {
        return;
    }

    const int length = static_cast<int>(line.length()) - breakPos;
    setFormat(breakPos, length, getBackgroundFormat(Qt::red));
}

void RemoteEditHighlighter::highlightAnsi(const QString &line)
{
    const auto red = getBackgroundFormat(Qt::red);
    const auto cyan = getBackgroundFormat(Qt::cyan);

    mmqt::foreachAnsi(line, [this, &red, &cyan](const auto start, const QStringView sv) {
        setFormat(static_cast<int>(start),
                  static_cast<int>(sv.length()),
                  mmqt::isValidAnsiColor(sv) ? cyan : red);
    });
}

void RemoteEditHighlighter::highlightEntities(const QString &line)
{
    const auto red = getBackgroundFormat(Qt::red);
    const auto darkOrange = getBackgroundFormat(darkOrangeQColor);
    const auto yellow = getBackgroundFormat(Qt::yellow);

    struct NODISCARD MyEntityCallback final : entities::EntityCallback
    {
    private:
        RemoteEditHighlighter &m_self;
        const QTextCharFormat &m_red;
        const QTextCharFormat &m_darkOrange;
        const QTextCharFormat &m_yellow;

    public:
        explicit MyEntityCallback(RemoteEditHighlighter &self,
                                  const QTextCharFormat &red,
                                  const QTextCharFormat &darkOrange,
                                  const QTextCharFormat &yellow)
            : m_self{self}
            , m_red{red}
            , m_darkOrange{darkOrange}
            , m_yellow{yellow}
        {}

    private:
        void virt_decodedEntity(int start, int len, OptQChar decoded) final
        {
            const auto get_fmt = [this, &decoded]() -> const QTextCharFormat & {
                if (!decoded) {
                    return m_red;
                }
                const auto val = decoded->unicode();
                return (val < 256) ? m_yellow : m_darkOrange;
            };

            m_self.setFormat(start, len, get_fmt());
        }
    } callback{*this, red, darkOrange, yellow};
    entities::foreachEntity(QStringView{line}, callback);
}

void RemoteEditHighlighter::highlightEncodingErrors(const QString &line)
{
    const auto red = getBackgroundFormat(Qt::red);
    const auto darkOrange = getBackgroundFormat(darkOrangeQColor);
    const auto yellow = getBackgroundFormat(Qt::yellow);
    const auto cyan = getBackgroundFormat(Qt::cyan);

    using OptFmt = std::optional<QTextCharFormat>;
#define DECL_FMT(name, color, tooltip) \
    OptFmt opt_##name{}; \
    const auto get_##name##_fmt = [&opt_##name, &color]() -> const QTextCharFormat & { \
        auto &opt = opt_##name; \
        if (!opt) { \
            opt = color; \
            if (USE_TOOLTIPS) { \
                opt.value().setToolTip(tooltip); \
            } \
        } \
        return opt.value(); \
    }
    DECL_FMT(unicode, red, "Disallowed Unicode");
    DECL_FMT(nbsp, cyan, "NBSP");
    DECL_FMT(unprintable, darkOrange, "(unprintable)");

#undef DECL_FMT

    int pos = 0;
    const int len = static_cast<int>(line.length());
    while (pos < len) {
        char32_t cp = 0;
        int charLen = 1;
        const QChar qc = line.at(pos);
        if (qc.isHighSurrogate() && pos + 1 < len && line.at(pos + 1).isLowSurrogate()) {
            cp = QChar::surrogateToUcs4(qc, line.at(pos + 1));
            charLen = 2;
        } else {
            cp = qc.unicode();
        }

        if (!m_options.isCodepointAllowed(cp)) {
            setFormat(pos, charLen, get_unicode_fmt());
        } else if (cp <= 0xFF) {
            const char c = static_cast<char>(cp);
            if (c == C_NBSP) {
                setFormat(pos, 1, get_nbsp_fmt());
            } else if (c != C_ESC && c != C_TAB) {
                if (ascii::isCntrl(c) || !::isPrintLatin1(c)) {
                    setFormat(pos, 1, get_unprintable_fmt());
                }
            }
        }

        pos += charLen;
    }
}
