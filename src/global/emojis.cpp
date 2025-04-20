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

// TODO: move this somewhere better?
NODISCARD bool isAsciiOrLatin1ControlCode(const char32_t codepoint)
{
    return (codepoint < 0x20u) || (codepoint >= 0x7Fu && codepoint < 0xA0u);
}

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

enum class NODISCARD CodePointType : uint8_t {
    Valid,
    Surrogate,
    Noncharacter,
    PrivateUse,
    OutOfBounds,
};

NODISCARD constexpr CodePointType classifyCodepoint(const char32_t codepoint)
{
#define X_EXCLUDE_RANGE(_lo, _hi, _reason) \
    static_assert((_lo) < (_hi)); \
    if (isClamped<char32_t>(codepoint, (_lo), (_hi))) { \
        return (CodePointType::_reason); \
    }

    if (codepoint > MAX_UNICODE_CODEPOINT) {
        return CodePointType::OutOfBounds;
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
        return CodePointType::Noncharacter;
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
        return CodePointType::PrivateUse;

    default:
        break;
    }

    return CodePointType::Valid;
#undef X_EXCLUDE_RANGE
}

NODISCARD constexpr bool isValidUnicode(const char32_t codepoint)
{
    return classifyCodepoint(codepoint) == CodePointType::Valid;
}

static_assert(isValidUnicode(0x0u));
static_assert(isValidUnicode(char_consts::thumbs_up));
static_assert(classifyCodepoint(INVALID_CODEPOINT) == CodePointType::OutOfBounds);

static_assert(classifyCodepoint(0xD800u) == CodePointType::Surrogate);
static_assert(classifyCodepoint(0xDFFFu) == CodePointType::Surrogate);

static_assert(classifyCodepoint(0xE000u) == CodePointType::PrivateUse);
static_assert(classifyCodepoint(0xEFFFu) == CodePointType::PrivateUse);

static_assert(classifyCodepoint(0xF000u) == CodePointType::PrivateUse);
static_assert(classifyCodepoint(0xF8FFu) == CodePointType::PrivateUse);

static_assert(isValidUnicode(0xFDCFu));
static_assert(isValidUnicode(0xFE0Fu));
static_assert(classifyCodepoint(0xFDD0u) == CodePointType::Noncharacter);
static_assert(classifyCodepoint(0xFFFFu) == CodePointType::Noncharacter);
static_assert(isValidUnicode(0x10000u));

static_assert(classifyCodepoint(0x10FFFDu) == CodePointType::PrivateUse);
static_assert(classifyCodepoint(0x10FFFFu) == CodePointType::Noncharacter);

NODISCARD std::optional<char32_t> tryGetOneCodepointHexCode(const QString &hex)
{
    uint64_t result = 0u;
    for (const QChar c : hex) {
        if (result > MAX_UNICODE_CODEPOINT) {
            return std::nullopt;
        }
        result *= 16u;
        if ('0' <= c && c <= '9') {
            result += c.unicode() - '0';
        } else if ('a' <= c && c <= 'f') {
            result += 10 + (c.unicode() - 'a');

        } else if ('A' <= c && c <= 'F') {
            result += 10 + (c.unicode() - 'A');
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

    return std::move(result);
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
    if (!s.contains(C_COLON)) {
        return s;
    }

    // REVISIT: check loaded emojis against the same pattern?
    static const QRegularExpression shortCodeRegex{":[-_+A-Za-z0-9]+:"};
    static const QRegularExpression unicodeRegex{"^[Uu]\\+([0-9A-Fa-f]+)$"};

    QString result;
    auto check = [&result](const QStringView match) -> bool {
        const auto &map = getEmojis().shortCodeToHex;
        assert(match.startsWith(C_COLON));
        assert(match.endsWith(C_COLON));
        const auto code = match.mid(1).chopped(1); // remove the colons
        if (const auto it = map.find(code.toString()); it != map.end()) {
            result += it->second;
            return true;
        }
        if (const auto m = unicodeRegex.match(code); m.hasMatch()) {
            const auto hex = m.captured(1);
            if (const auto opt = tryGetOneCodepointHexCode(hex)) {
                const char32_t codepoint = *opt;
                // also avoid latin-1 control codes
                // what about things like nbsp (U+A0), zwsp (U+200B), etc?
                if (codepoint > NUM_LATIN1_CODEPOINTS || !isAsciiOrLatin1ControlCode(codepoint)) {
                    result += QString(codepoint);
                    return true;
                }
            }
        }
        return false;
    };

    QStringView sv = s;
    auto advance_to_colon = [&result, &sv]() {
        if (const auto colon = sv.indexOf(C_COLON); colon != -1) {
            result += sv.left(colon);
            sv = sv.mid(colon);
            return;
        }
        result += sv;
        sv = {};
    };
    advance_to_colon();
    assert(sv.front() == C_COLON);

    // minimum short code length is 3 (includes two colons)
    constexpr qsizetype MIN_LEN = 3;
    while (sv.size() >= MIN_LEN && sv.front() == C_COLON) {
        const auto next_colon = sv.indexOf(C_COLON, 1);
        if (next_colon == -1) {
            break;
        }

        // maybe is ":text:"
        const auto maybe = sv.left(next_colon + 1);
        if (maybe.size() >= MIN_LEN && shortCodeRegex.match(maybe).hasMatch() && check(maybe)) {
            // replacement consumes the first and second colons
            sv = sv.mid(next_colon + 1);
            // look for the next colon after the two that were just matched
            advance_to_colon();
            continue;
        }

        // otherwise the 2nd colon survives as the first colon of the next potential match
        result += sv.left(next_colon);
        sv = sv.mid(next_colon);
    }
    if (!sv.empty()) {
        result += sv;
    }
    return result;
}

NODISCARD QString mmqt::encodeEmojiShortCodes(const QString &s)
{
    if (!containsNonLatin1Codepoints(s)) {
        return s;
    }

    static constexpr auto colon = static_cast<char32_t>(mmqt::QC_COLON.unicode());
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
            snprintf(hex, sizeof(hex), ":U+%X:", c);
            for (const char ascii : std::string_view{hex}) {
                m_output += static_cast<char32_t>(static_cast<uint8_t>(ascii));
            }
        }
        void virt_report(std::u32string_view sv) const final
        {
            m_output += colon;
            m_output += sv;
            m_output += colon;
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
    importEmojis(R"({"+1": "1F44D", "100": "1F4AF"})", "test-input");
    struct NODISCARD TestCase final
    {
        QString input;
        QString expected;
        bool roundtrip = true;
    };
    const std::vector<TestCase> testCases{TestCase{":0:+1:", ":0\U0001F44D"},
                                          {":100:+1:100:", "\U0001F4AF+1\U0001F4AF"},
                                          {":::100::::+1:::", "::\U0001F4AF::\U0001F44D::"},
                                          // :a: and :b: are in the default short-code database,
                                          // they're skipped here so that copy/pasting these
                                          // test cases into mmapper will give the same results.
                                          {":c:e:f:g:h:i:", ":c:e:f:g:h:i:"},
                                          {":", ":"},
                                          {"::", "::"},
                                          {":::", ":::"},
                                          // these cases can't roundtrip, because :u+ff:
                                          // is a synthetic short code that's only decoded.
                                          {":u+0:u+ff:", ":u+0\u00FF", false},
                                          {":u+ff:u+ff:", "\u00FFu+ff:", false}};
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
