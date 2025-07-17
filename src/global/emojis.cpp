// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include "emojis.h"

#include "Charset.h"
#include "ConfigConsts.h"
#include "TaggedInt.h"
#include "TextUtils.h"
#include "entities.h"
#include "utils.h"

#include <map>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <QChar>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

namespace {

const volatile bool verbose_debugging = false;
using charset::charset_detail::NUM_LATIN1_CODEPOINTS;
constexpr auto INVALID_CODEPOINT = ~char32_t(0);

// TODO: move this somewhere and add tests
namespace unicode {
NODISCARD bool isSurrogate(const char32_t c)
{
    return c >= 0xD800u && c <= 0xDFFFu;
}

NODISCARD bool isPrivateUse(const char32_t c)
{
    return (c >= 0xE000u && c <= 0xF8FFu)          // BMP
           || (c >= 0xF'0000u && c <= 0xF'FFFDu)   // PUP (15)
           || (c >= 0x10'0000 && c <= 0x10'FFFDu); // PUP (16)
}

NODISCARD bool isNonCharacter(const char32_t c)
{
    // special case for Plane 0 (BMP)
    if (c >= 0xFDD0u && c <= 0xFDEFu) {
        return true;
    }

    // The top 2 codepoints are invalid in all planes.
    switch (c & 0xFFFFu) {
    case 0xFFFEu:
    case 0xFFFFu:
        return true;
    default:
        return false;
    }
}

NODISCARD bool isInvalidUnicode(const char32_t c)
{
    return c > MAX_UNICODE_CODEPOINT || isSurrogate(c) || isPrivateUse(c) || isNonCharacter(c);
}

} // namespace unicode

// TODO: move this somewhere better?
NODISCARD bool isAsciiOrLatin1ControlCode(const char32_t codepoint)
{
    return (codepoint < 0x20u) || (codepoint >= 0x7Fu && codepoint < 0xA0u);
}

NODISCARD constexpr bool blacklisted(const char32_t c)
{
    return (c <= 0x00FFu)                    // (forbidden to avoid ambiguity)
           || (c >= 0x0180u && c <= 0x024Fu) // Latin extended B (non-european)
           || (c >= 0x0300u && c <= 0x036Fu) // spacing modifiers and diacritical marks ("insanity")
           || (c >= 0x2800u && c <= 0x28FFu) // braille
           || (c >= 0x2C00u && c <= 0x2FDFu); // non-western scripts
}

// This is a tolerant whitelist that may include codepoints we don't want;
// consider using a table of assigned emojis.
NODISCARD constexpr bool whitelisted(const char32_t c)
{
    if (blacklisted(c)) {
        return false;
    }

    // see:
    // https://en.wikipedia.org/wiki/Latin_Extended-A
    // https://en.wikipedia.org/wiki/General_Punctuation
    // https://en.wikipedia.org/wiki/Emoji#In_Unicode
    //
    // As of April 2025, there are two latin-1 emojis (U+00A9 and U+00AE),
    // four japanese emojis (U+3030, U+303D, U+3297, and U+3299)
    // and all the rest live in either U+2000..U+2FFF or U+1F000..U+1FFFF.
    //
    // https://en.wikibooks.org/wiki/Unicode/Character_reference/0000-0FFF
    // https://en.wikibooks.org/wiki/Unicode/Character_reference/2000-2FFF
    // https://en.wikibooks.org/wiki/Unicode/Character_reference/1F000-1FFFF
    //

    switch (c >> 12u) {
    case 0x0u:
        // purposely ignores Latin-1 (code points 0-255),
        // as well as latin-extended B (non-western),
        // spacing modifiers, and diacritical marks ("insanity").
        return c >= 0x0100u && c <= 0x017Fu; // latin extended A
    case 0x2u:
        return true; // punct and emoji
    case 0x3u:
        switch (c >> 4u) {
        case 0x303u:
        case 0x329u:
            return true; // japanese emojis
        default:
            return false;
        }
    case 0xF:
        return c == 0xFE0Fu; // found in various short codes
    case 0x1Fu:
        return true; // emoji
    case 0xE0:
        return (c >= 0xE0000u && c <= 0xE007Fu)     // found in short codes for flags
               || (c >= 0xE0100u && c <= 0xE01EFu); // variation selectors
    default:
        return false;
    }
}

static_assert(whitelisted(0x0100u));
static_assert(whitelisted(0x017Fu));
static_assert(blacklisted(0x0180u));
//
static_assert(blacklisted(0x0300u));
static_assert(blacklisted(0x036Fu));
//
static_assert(whitelisted(0x2000u));
static_assert(whitelisted(0x27FFu));
//
static_assert(blacklisted(0x2800u));
static_assert(blacklisted(0x28FFu));
//
static_assert(whitelisted(0x2900u));
static_assert(whitelisted(0x2BFFu));
//
static_assert(blacklisted(0x2C00u));
static_assert(blacklisted(0x2FDFu));
//
static_assert(whitelisted(0x2FF0u));
//
static_assert(whitelisted(0x3030u));
static_assert(whitelisted(0x303Du));
static_assert(whitelisted(0x3297u));
static_assert(whitelisted(0x3299u));
//
static_assert(whitelisted(0xFE0Fu));
//
static_assert(whitelisted(0x1F000u));
//
static_assert(whitelisted(0xE0062u));
static_assert(whitelisted(0xE007Fu));
static_assert(whitelisted(0xE0100u));

struct NODISCARD Emojis final
{
public:
    class NODISCARD HexPrefixTree final
    {
    private:
#if defined(__cpp_lib_generic_unordered_lookup) && __cpp_lib_generic_unordered_lookup
        static_assert(false, "Not tested; this may need to be fixed.");
        using HashBase = std::hash<std::u32string_view>;
        struct NODISCARD Hash final : public HashBase
        {
            auto operator()(const std::u32string &s) const { return HashBase::operator()(s); }
        };
        struct NODISCARD Pred final
        {
            using T1 = const std::u32string &;
            using T2 = const std::u32string_view;
            bool operator()(T1 a, T1 b) const { return a == b; }
            bool operator()(T1 a, T2 b) const { return a == b; }
            bool operator()(T2 a, T1 b) const { return a == b; }
            // bool operator()(T2 a, T2 b) const { return a == b; }
        };
        using Map = std::unordered_map<std::u32string, std::optional<std::u32string>, Hash, Pred>;
#else
        struct NODISCARD Comp final
        {
            using is_transparent = void;
            using T1 = const std::u32string &;
            using T2 = const std::u32string_view;
            bool operator()(T1 a, T1 b) const { return a < b; }
            bool operator()(T1 a, T2 b) const { return a < b; }
            bool operator()(T2 a, T1 b) const { return a < b; }
            // bool operator()(T2 a, T2 b) const { return a < b; }
        };
        using Map = std::map<std::u32string, std::optional<std::u32string>, Comp>;
#endif

        Map m_map;

        void try_insert(const std::u32string_view key,
                        const std::u32string_view replacement,
                        const bool is_final)
        {
            assert(replacement.empty() ^ is_final);

            const auto it = m_map.emplace(key, std::nullopt).first;
            assert(it != m_map.end());
            if (is_final) {
                auto &value = it->second;
                if (!value.has_value() || replacement.size() < value.value().size()) {
                    value = replacement;
                }
            }
        }

    public:
        NODISCARD bool is_valid_prefix(const std::u32string_view key) const
        {
            return m_map.find(key) != m_map.end();
        }

        NODISCARD const std::u32string *lookup_replacement(const std::u32string_view key) const
        {
            if (const auto it = m_map.find(key); it != m_map.end()) {
                if (const auto &opt = it->second; opt.has_value()) {
                    return &opt.value();
                }
            }
            return nullptr;
        }

        void insert(const std::u32string_view key, const std::u32string_view replacement)
        {
            for (size_t i = 1; i < key.size(); ++i) {
                try_insert(key.substr(0, i), {}, false);
            }
            try_insert(key, replacement, true);

            if constexpr (IS_DEBUG_BUILD) {
                for (size_t i = 1; i < key.size(); ++i) {
                    assert(is_valid_prefix(key.substr(0, i)));
                }
                const auto actual = lookup_replacement(key);
                assert(actual != nullptr); /* note: ours might be longer */
                assert(deref(actual).size() <= replacement.size());
            }
        }

        struct Output
        {
            virtual ~Output() = default;
            void operator()(const char32_t c) const
            {
                assert(c != INVALID_CODEPOINT);
                virt_report(c);
            }
            void operator()(const std::u32string_view sv) const
            {
                if constexpr (IS_DEBUG_BUILD) {
                    for (const auto c : sv) {
                        assert(c != INVALID_CODEPOINT);
                    }
                }
                virt_report(sv);
            }

        private:
            virtual void virt_report(char32_t c) const = 0;
            virtual void virt_report(std::u32string_view sv) const = 0;
        };

        class NODISCARD Matcher final
        {
        private:
            std::u32string m_input;

        public:
            void check(const HexPrefixTree &tree,
                       const char32_t input_codepoint,
                       const Output &output)
            {
                std::u32string stack;
                stack += input_codepoint;

                auto grow = [this, &stack]() {
                    assert(!stack.empty());
                    m_input += utils::pop_back(stack);
                };
                auto shrink = [this, &stack]() {
                    assert(!m_input.empty());
                    stack += utils::pop_back(m_input);
                };

                while (!stack.empty()) {
                    grow();
                    if (tree.is_valid_prefix(m_input)) {
                        continue;
                    }

                    // finally no match on the look-ahead(1).
                    shrink();

                    bool replaced = false;
                    for (; !m_input.empty(); shrink()) {
                        assert(tree.is_valid_prefix(m_input));
                        // loop until we find an actual replacement
                        if (const std::u32string *const repl = tree.lookup_replacement(m_input);
                            repl != nullptr) {
                            replaced = true;
                            m_input.clear();
                            output(*repl);
                            break;
                        }
                    }
                    if (!replaced) {
                        // satisfy the loop invariant by removing the unmatched element
                        assert(m_input.empty());
                        const auto unmatched = utils::pop_back(stack);
                        if (unmatched != INVALID_CODEPOINT) {
                            output(unmatched);
                        }
                    }
                }
            }
            void flush(const HexPrefixTree &tree, const Output &output)
            {
                check(tree, INVALID_CODEPOINT, output);
            }
        };
    };

public:
    std::map<QString, QString> shortCodeToHex;
    HexPrefixTree hexPrefixTree;

public:
    void reset()
    {
        *this = {};
    }
};

NODISCARD Emojis &getEmojis()
{
    static Emojis g_emojis;
    return g_emojis;
}

enum class NODISCARD CodePointTypeEnum : uint8_t {
    Valid,
    Surrogate,
    Noncharacter,
    PrivateUse,
    OutOfBounds,
};

NODISCARD constexpr CodePointTypeEnum classifyCodepoint(const char32_t codepoint)
{
#define X_EXCLUDE_RANGE(_lo, _hi, _reason) \
    static_assert((_lo) < (_hi)); \
    if (isClamped<char32_t>(codepoint, (_lo), (_hi))) { \
        return (CodePointTypeEnum::_reason); \
    }

    if (codepoint > MAX_UNICODE_CODEPOINT) {
        return CodePointTypeEnum::OutOfBounds;
    }

    // https://en.wikipedia.org/wiki/Universal_Character_Set_characters#Surrogates
    // 0xD800-0xDFFF
    // https://en.wikipedia.org/wiki/Universal_Character_Set_characters#Noncharacters
    // U+FDD0..U+FDEF
    // U+FFFE and U+FFFF (plus the 16 planes all reserve these as noncharacters)
    // https://en.wikipedia.org/wiki/Private_Use_Areas
    // U+E000..U+F8FF
    // U+F0000..U+FFFFD
    // U+100000..U+10FFFD

    if ((codepoint & 0xFFFFu) >= 0xFFFEu && isClamped(codepoint >> 16u, 0u, 16u)) {
        // U+xxXXFE and U+xxXXFF
        return CodePointTypeEnum::Noncharacter;
    }
    switch (codepoint >> 16u) {
    case 0x0u:
        switch ((codepoint >> 12u) & 0xFu) {
        case 0xDu:
            X_EXCLUDE_RANGE(0xD800u, 0xDFFFu, Surrogate);
            break;
        case 0xEu:
            X_EXCLUDE_RANGE(0xE000u, 0xEFFFu, PrivateUse);
            break;
        case 0xFu:
            X_EXCLUDE_RANGE(0xF000u, 0xF8FFu, PrivateUse);
            X_EXCLUDE_RANGE(0xFDD0u, 0xFDEFu, Noncharacter);
            break;
        }
        break;

    case 0xFu:
    case 0x10u:
        return CodePointTypeEnum::PrivateUse;

    default:
        break;
    }

    return CodePointTypeEnum::Valid;
#undef X_EXCLUDE_RANGE
}

NODISCARD constexpr bool isValidUnicode(const char32_t codepoint)
{
    return classifyCodepoint(codepoint) == CodePointTypeEnum::Valid;
}

static_assert(isValidUnicode(0x0u));
static_assert(isValidUnicode(char_consts::thumbs_up));
static_assert(classifyCodepoint(INVALID_CODEPOINT) == CodePointTypeEnum::OutOfBounds);

static_assert(classifyCodepoint(0xD800u) == CodePointTypeEnum::Surrogate);
static_assert(classifyCodepoint(0xDFFFu) == CodePointTypeEnum::Surrogate);

static_assert(classifyCodepoint(0xE000u) == CodePointTypeEnum::PrivateUse);
static_assert(classifyCodepoint(0xEFFFu) == CodePointTypeEnum::PrivateUse);

static_assert(classifyCodepoint(0xF000u) == CodePointTypeEnum::PrivateUse);
static_assert(classifyCodepoint(0xF8FFu) == CodePointTypeEnum::PrivateUse);

static_assert(isValidUnicode(0xFDCFu));
static_assert(isValidUnicode(0xFE0Fu));
static_assert(classifyCodepoint(0xFDD0u) == CodePointTypeEnum::Noncharacter);
static_assert(classifyCodepoint(0xFFFFu) == CodePointTypeEnum::Noncharacter);
static_assert(isValidUnicode(0x10000u));

static_assert(classifyCodepoint(0x10FFFDu) == CodePointTypeEnum::PrivateUse);
static_assert(classifyCodepoint(0x10FFFFu) == CodePointTypeEnum::Noncharacter);

NODISCARD std::optional<char32_t> tryGetOneCodepointHexCode(const QString &hex)
{
    uint64_t result = 0u;
    for (const QChar c : hex) {
        if (result > MAX_UNICODE_CODEPOINT) {
            return std::nullopt;
        }
        result *= 16u;
        if ('0' <= c && c <= '9') {
            result += c.unicode() - static_cast<uint32_t>(u'0');
        } else if ('a' <= c && c <= 'f') {
            result += 10 + (c.unicode() - static_cast<uint32_t>(u'a'));

        } else if ('A' <= c && c <= 'F') {
            result += 10 + (c.unicode() - static_cast<uint32_t>(u'A'));
        } else {
            return std::nullopt;
        }
    }
    const auto c = static_cast<char32_t>(result);
    if (!isValidUnicode(c)) {
        return std::nullopt;
    }
    return c;
}

NODISCARD std::optional<std::u32string> getUnicode(const QString &hex)
{
    std::u32string result;
    for (const QString &s : hex.split("-")) {
        if (auto opt = tryGetOneCodepointHexCode(s)) {
            result += opt.value();
        } else {
            return std::nullopt;
        }
    }

    return std::optional<std::u32string>{std::move(result)};
}

NODISCARD bool containsSurrogates(const QString &s)
{
    for (const QChar c : s) {
        if (c.isSurrogate()) {
            return true;
        }
    }
    return false;
}

} // namespace

NODISCARD bool mmqt::containsNonLatin1Codepoints(const QString &s)
{
    for (const QChar c : s) {
        if (c.unicode() > NUM_LATIN1_CODEPOINTS) {
            return true;
        }
    }
    return false;
}

NODISCARD QString mmqt::decodeEmojiShortCodes(const QString &s)
{
    using namespace char_consts;
    if (!s.contains(QStringLiteral("[:"))) {
        return s;
    }

    // REVISIT: check loaded emojis against the same pattern?
    static const QRegularExpression shortCodeRegex{R"(\[:[-_+A-Za-z0-9]+\:])"};
    static const QRegularExpression unicodeRegex{R"(^[Uu]\+([0-9A-Fa-f]+)$)"};

    QString result;
    QStringView view(s);
    qsizetype lastPos = 0;

    QRegularExpressionMatchIterator it = shortCodeRegex.globalMatch(s);

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        const auto matchStart = match.capturedStart();
        const auto matchEnd = match.capturedEnd();

        result += view.mid(lastPos, matchStart - lastPos);

        QString inside = view.mid(matchStart + 2, matchEnd - matchStart - 4)
                             .toString(); // Skip [: and :]
        const auto &map = getEmojis().shortCodeToHex;
        const auto emojiIt = map.find(inside);
        if (emojiIt != map.end()) {
            result += emojiIt->second;
        } else {
            const auto unicodeMatch = unicodeRegex.match(inside);
            if (unicodeMatch.hasMatch()) {
                const QString hex = unicodeMatch.captured(1);
                if (const auto opt = tryGetOneCodepointHexCode(hex)) {
                    const char32_t codepoint = *opt;

                    // note: we forbid *all* Latin-1 codepoints, instead of just control codes,
                    // so there won't ever be any ambiguity over XML, ansi, etc.
                    if (codepoint <= NUM_LATIN1_CODEPOINTS || unicode::isInvalidUnicode(codepoint)
                        || !whitelisted(codepoint)) {
                        result += view.mid(matchStart, matchEnd - matchStart); // Invalid codepoint
                    } else {
                        result += QString::fromUcs4(&codepoint, 1);
                    }
                } else {
                    result += view.mid(matchStart, matchEnd - matchStart); // Invalid hex
                }
            } else {
                result += view.mid(matchStart, matchEnd - matchStart); // Unknown shortcode
            }
        }

        lastPos = matchEnd;
    }

    result += view.mid(lastPos);
    return result;
}

NODISCARD QString mmqt::encodeEmojiShortCodes(const QString &s)
{
    if (!containsNonLatin1Codepoints(s)) {
        return s;
    }
    static constexpr auto close_bracket = static_cast<char32_t>(mmqt::QC_CLOSE_BRACKET.unicode());
    static constexpr auto colon = static_cast<char32_t>(mmqt::QC_COLON.unicode());
    static constexpr auto open_bracket = static_cast<char32_t>(mmqt::QC_OPEN_BRACKET.unicode());
    static constexpr auto question_mark = static_cast<char32_t>(mmqt::QC_QUESTION_MARK.unicode());
    const auto sv = std::u16string_view{reinterpret_cast<const char16_t *>(s.constData()),
                                        static_cast<size_t>(s.size())};

    std::u32string output;
    output.reserve(sv.size());

    const auto &emojis = getEmojis();

    Emojis::HexPrefixTree::Matcher matcher;
    class Output : public Emojis::HexPrefixTree::Output
    {
    private:
        std::u32string &m_output;

    public:
        explicit Output(std::u32string &o)
            : m_output{o}
        {}

    private:
        void virt_report(const char32_t c) const final
        {
            if (c <= NUM_LATIN1_CODEPOINTS) {
                m_output += c;
                return;
            }

            if (!isValidUnicode(c)) {
                m_output += question_mark;
                return;
            }

            // REVISIT: should this include "U+" or not?
            char hex[32];
            snprintf(hex, sizeof(hex), "[:U+%X:]", c);
            for (const char ascii : std::string_view{hex}) {
                m_output += static_cast<char32_t>(static_cast<uint8_t>(ascii));
            }
        }
        void virt_report(std::u32string_view sv) const final
        {
            m_output += open_bracket;
            m_output += colon;
            m_output += sv;
            m_output += colon;
            m_output += close_bracket;
        }
    };

    Output callback{output};
    auto lambda = [&emojis, &matcher, &callback](const char32_t codepoint) {
        matcher.check(emojis.hexPrefixTree, codepoint, callback);
    };
    charset::conversion::utf16::foreach_codepoint_utf16(sv, lambda);
    matcher.flush(emojis.hexPrefixTree, callback);
    return QString::fromStdU32String(output);
}

static void importEmojis(const QByteArray &bytes, const QString &filename)
{
    QJsonParseError err{0, QJsonParseError::NoError};
    const auto doc = QJsonDocument::fromJson(bytes, &err);
    if (!doc.isObject()) {
        qWarning() << "File" << filename << "does not contain a JSON object.";
        return;
    }
    const auto obj = doc.object();
    const auto num_input_emojis = obj.size();
    qInfo().nospace() << "Reading " << num_input_emojis << " emojis from " << filename << "...";

    auto &emojis = getEmojis();
    emojis.reset();
    size_t num_output_emojis = 0;
    auto report = [&](const QString &shortCode, const QString &hex) {
        if (auto opt_emoji = getUnicode(hex)) {
            for (const char32_t c : *opt_emoji) {
                if (isAsciiOrLatin1ControlCode(c)) {
                    // REVISIT: strongly consider rejecting the short code?
                    qWarning().nospace() << "Short code " << shortCode << " = " << hex
                                         << " contains a control code.";
                    break;
                } else if (c > NUM_LATIN1_CODEPOINTS && !whitelisted(c)) {
                    // REVISIT: consider rejecting the short code?
                    qWarning().nospace() << "Short code " << shortCode << " = " << hex
                                         << " contains non-whitelisted codepoint " << c
                                         << " (aka U+"
                                         /* << Qt::uppercase */
                                         << Qt::hex << static_cast<uint32_t>(c) << ")";
                    break;
                }
            }
            const auto qemoji = QString::fromStdU32String(*opt_emoji);
            if (verbose_debugging) {
                qDebug() << "shortCode" << shortCode << "emoji" << qemoji;
            }
            emojis.shortCodeToHex.try_emplace(shortCode, qemoji);
            emojis.hexPrefixTree.insert(*opt_emoji, shortCode.toStdU32String());
        } else {
            qWarning() << "failed to translate shortCode" << shortCode << "from hex" << hex;
        }
    };

    for (const QString &alias : obj.keys()) {
        if (alias.isNull() || alias.isEmpty()) {
            continue;
        }
        if (containsSurrogates(alias)) {
            if (verbose_debugging) {
                qWarning() << "alias" << alias;
            }
            continue;
        }
        const auto hexValue = obj[alias];
        if (!hexValue.isString()) {
            if (verbose_debugging) {
                qWarning() << "value" << hexValue;
            }
            continue;
        }
        ++num_output_emojis;
        report(alias, hexValue.toString());
    }
    qInfo().nospace() << "Read " << num_output_emojis << " emoji aliases from " << filename << ".";
}

void tryLoadEmojis(const QString &filename)
{
    QFile file{filename};
    if (!file.exists()) {
        qWarning() << "File" << filename << "does not exist.";
        return;
    }
    if (!file.open(QFile::OpenModeFlag::ReadOnly)) {
        qWarning() << "Unable to open file" << filename << "for reading.";
        return;
    }
    const QByteArray bytes = file.readAll();
    if (bytes.isNull() || bytes.isEmpty()) {
        qWarning() << "Unable to read file" << filename << "contents.";
        return;
    }

    importEmojis(bytes, filename);
}

void test::test_emojis()
{
    importEmojis(R"({"+1": "1F44D", "100": "1F4AF", "a": "1F170"})", "test-input");
    struct NODISCARD TestCase final
    {
        QString input;
        QString expected;
        bool roundtrip = true;
    };

    const std::vector<TestCase> testCases{
        // Positive cases
        {"[:+1:]", "\U0001F44D"},                         // thumbs up
        {"[:+1:][:100:]", "\U0001F44D\U0001F4AF"},        // thumbs up and 100
        {"[:U+1F44D:]", "\U0001F44D", false},             // thumbs up
        {"[:u+1f44d:]", "\U0001F44D", false},             // thumbs up (lowercase)
        {"[:u+0001F44D:]", "\U0001F44D", false},          // thumbs up (with 0s)
        {"[:a:][:c:]", "\U0001F170[:c:]"},                // a and unknown shortcode
        {"[:c:][:e:][:f:][:g:]", "[:c:][:e:][:f:][:g:]"}, // unknown shortcodes, passed through
        {"[:foo:] text [:bar:]", "[:foo:] text [:bar:]"}, // unknown shortcodes, passed through
        {"[:1F44D:]", "[:1F44D:]"},                       // unknown shortcode (no U+)
        {"[:1f44d:]", "[:1f44d:]"},                       // unknown shortcode (no U+; lowercase)
        {"[:U+0061:]", "[:U+0061:]"},                     // disallowed ASCII 'a'
        {"[:U+61:]", "[:U+61:]"},                         // disallowed ASCII 'a' (no 0s)

        // Edge cases - valid partial matches
        {":+1[:+1:]", ":+1\U0001F44D"},
        {":[+1[:+1:]", ":[+1\U0001F44D"},
        {"[:+1:]+1:", "\U0001F44D+1:"},
        {"[:+1:]+1:]", "\U0001F44D+1:]"},
        {"[:100:]+1[:100:]", "\U0001F4AF+1\U0001F4AF"},
        {"::[:100:]::[:+1:]::", "::\U0001F4AF::\U0001F44D::"},

        // Raw text edge cases
        {"[:", "[:"},
        {":]", ":]"},
        {"[::]", "[::]"},
        {"[:[:]:]", "[:[:]:]"},
        {"[:[::]:]", "[:[::]:]"},

        // Negative cases - invalid parsing, pass-through
        {"[:U+110000:]", "[:U+110000:]"}, // Unicode > 0x10FFFF invalid
        {"[:U+ZZZZ:]", "[:U+ZZZZ:]"},     // invalid hex
        {"[:100a:]", "[:100a:]"},         // extra words
        {"[:+1", "[:+1"},                 // missing closing colon
        {":+1:]", ":+1:]"},               // missing opening bracket
        {"[:U+1F44D", "[:U+1F44D"},       // missing closing colon
        {":U+1F44D:]", ":U+1F44D:]"},     // missing opening bracket
        {"[:U+:]", "[:U+:]"},             // missing number after U+
    };

    for (const TestCase &tc : testCases) {
        if (const auto decoded = mmqt::decodeEmojiShortCodes(tc.input); decoded != tc.expected) {
            qInfo() << "[decode] input:" << tc.input << "expected:" << tc.expected
                    << "but decoded:" << decoded;
            std::abort();
        }
        if (tc.roundtrip) {
            if (const auto encoded = mmqt::encodeEmojiShortCodes(tc.expected); encoded != tc.input) {
                qInfo() << "[encode] input:" << tc.expected << "expected:" << tc.input
                        << "but encoded:" << encoded;
                std::abort();
            }
        }
    }
}
