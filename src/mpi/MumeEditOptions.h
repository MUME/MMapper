#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "../global/macros.h"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <vector>

#include <QCoreApplication>
#include <QString>
#include <QStringList>

// These live outside MumeEditOptions on purpose: a nested struct with default member initializers
// can't be default-constructed while the enclosing class is still being defined, which breaks the
// std::optional<AllowedRanges> member below with libstdc++.
struct NODISCARD MumeEditRange final
{
    char32_t start = 0;
    char32_t end = 0;
};

// The restriction GMCP sent in "allowed-ranges".
struct NODISCARD MumeAllowedRanges final
{
    std::vector<MumeEditRange> ranges;
    bool ansiSgr = false;
    // Parsed for completeness, but deliberately not used to widen what is accepted: MUME
    // still rewrites emoji and their modifiers that are not in the numeric ranges, so we only
    // promise what it lists explicitly.
    bool emoji = false;

    // Ignores malformed ranges (negative, reversed, or beyond Unicode).
    void tryAddRange(const int start, const int end)
    {
        if (start >= 0 && start <= end && end <= 0x10FFFF) {
            ranges.push_back({static_cast<char32_t>(start), static_cast<char32_t>(end)});
        }
    }
};

struct NODISCARD MumeEditOptions final
{
public:
    using Range = MumeEditRange;
    using AllowedRanges = MumeAllowedRanges;

public:
    // Remote edit uses Unicode and GMCP session constraints, independently of the client encoding.
    std::optional<int> maxSize;
    // Engaged if and only if GMCP sent an "allowed-ranges" key. Even an empty or fully malformed
    // list restricts; an absent key allows everything.
    std::optional<AllowedRanges> allowed;

public:
    // Ignores non-positive limits; the server may still reject the save.
    void trySetMaxSize(const int size)
    {
        if (size > 0) {
            maxSize = size;
        }
    }

public:
    // Zero-width characters that glue emoji sequences together (joiner, variation selectors,
    // keycap, tag characters). They are invisible, so the user can't see them to remove them.
    NODISCARD static bool isInvisibleEmojiModifier(const char32_t cp)
    {
        return cp == 0x200D || cp == 0xFE0E || cp == 0xFE0F || cp == 0x20E3
               || (cp >= 0xE0020 && cp <= 0xE007F);
    }

    NODISCARD bool isCodepointAllowed(const char32_t cp) const
    {
        if (!allowed) {
            return true;
        }

        if (allowed->ansiSgr && cp == 0x1B) {
            return true;
        }

        for (const auto &r : allowed->ranges) {
            if (cp >= r.start && cp <= r.end) {
                return true;
            }
        }
        return false;
    }

    // Distinct disallowed code points in order of appearance, stopping after `limit`.
    NODISCARD std::vector<char32_t> findDisallowedCodepoints(const QString &text,
                                                             const size_t limit) const
    {
        std::vector<char32_t> result;
        const int len = static_cast<int>(text.length());
        int pos = 0;
        while (pos < len && result.size() < limit) {
            char32_t cp = text.at(pos).unicode();
            int charLen = 1;
            if (text.at(pos).isHighSurrogate() && pos + 1 < len
                && text.at(pos + 1).isLowSurrogate()) {
                cp = QChar::surrogateToUcs4(text.at(pos), text.at(pos + 1));
                charLen = 2;
            }
            if (!isCodepointAllowed(cp)
                && std::find(result.begin(), result.end(), cp) == result.end()) {
                result.push_back(cp);
            }
            pos += charLen;
        }
        return result;
    }

    NODISCARD bool hasDisallowedCodepoints(const QString &text) const
    {
        return !findDisallowedCodepoints(text, 1).empty();
    }

    NODISCARD bool exceedsMaxSize(const QString &text) const
    {
        return maxSize.has_value() && text.toUtf8().size() > maxSize.value();
    }

    // Human-readable list of the problems with `text`, shared by the internal and external
    // editors; empty if there are none. Callers append their own question.
    NODISCARD QString describeIssues(const QString &text) const
    {
        const bool disallowed = hasDisallowedCodepoints(text);
        const bool overLimit = exceedsMaxSize(text);
        if (!disallowed && !overLimit) {
            return {};
        }

        const auto tr = [](const char *const s) {
            return QCoreApplication::translate("MumeEditOptions", s);
        };
        QString msg = tr("The text contains issues before submitting:\n");
        if (disallowed) {
            // Name the offenders: invisible ones (e.g. U+FE0F) can't be spotted in the text.
            constexpr size_t MAX_LISTED = 5;
            QStringList names;
            for (const char32_t cp : findDisallowedCodepoints(text, MAX_LISTED + 1)) {
                if (names.size() == MAX_LISTED) {
                    names << QStringLiteral("...");
                    break;
                }
                const QString code = QStringLiteral("U+%1")
                                         .arg(static_cast<uint>(cp), 4, 16, QLatin1Char('0'))
                                         .toUpper();
                const bool visible = !isInvisibleEmojiModifier(cp) && QChar::isPrint(cp);
                names << (visible ? QString::fromUcs4(&cp, 1) + QLatin1Char(' ') + code : code);
            }
            msg += tr("- Disallowed Unicode characters for this editor session: %1\n")
                       .arg(names.join(QStringLiteral(", ")));
        }
        if (overLimit) {
            msg += tr("- Exceeds maximum byte limit (%1 > %2)\n")
                       .arg(text.toUtf8().size())
                       .arg(maxSize.value());
        }
        return msg;
    }
};
