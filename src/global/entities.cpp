// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "entities.h"

#include "Consts.h"
#include "RuleOf5.h"

#include <cassert>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include <QByteArray>
#include <QHash>
#include <QString>

using namespace char_consts;

NODISCARD static bool isLatin1(const QChar qc)
{
    return qc.unicode() < 256;
}

NODISCARD static bool isLatin1Digit(const QChar qc)
{
    return isLatin1(qc) && isdigit(qc.toLatin1());
}

NODISCARD static bool isLatin1HexDigit(const QChar qc)
{
    return isLatin1(qc) && isxdigit(qc.toLatin1());
}

NODISCARD static bool isLatin1Alpha(const QChar qc)
{
    return isLatin1(qc) && isalpha(qc.toLatin1());
}

entities::EntityCallback::~EntityCallback() = default;

#include "entities-xforeach.txt"

namespace entities {

/*
 * https://www.w3.org/TR/REC-xml/#NT-S says:
 * > [4]  NameStartChar    ::= ":" | [A-Z] | "_" | [a-z] | [#xC0-#xD6] | [#xD8-#xF6] | [#xF8-#x2FF] | [#x370-#x37D] | [#x37F-#x1FFF] | [#x200C-#x200D] | [#x2070-#x218F] | [#x2C00-#x2FEF] | [#x3001-#xD7FF] | [#xF900-#xFDCF] | [#xFDF0-#xFFFD] | [#x10000-#xEFFFF]
 * > [4a] NameChar         ::=  NameStartChar | "-" | "." | [0-9] | #xB7 | [#x0300-#x036F] | [#x203F-#x2040]
 *
 * In the above, note that:
 * D7 (&times;) and F7 (&divide;) aren't valid NameStartChar or NameChar.
 * B7 (&middot;) is a valid NameChar
 * C0 (&Agrave;) .. F6 (&ouml;) are valid NameStartChar and NameChar, except for D7 and F7.
 */

// only bother with the latin1 subset
NODISCARD static bool isNameStartChar(const QChar c)
{
    if (c == C_COLON || c == C_UNDERSCORE || isLatin1Alpha(c)) {
        return true;
    }
    const auto uc = static_cast<uint32_t>(c.unicode());
    return uc >= 0xc0 && uc != 0xd7 && uc != 0xf7;
}

// only bother with the latin1 subset
NODISCARD static bool isNameChar(const QChar c)
{
    return isNameStartChar(c) || c == C_MINUS_SIGN || c == C_PERIOD || isLatin1Digit(c)
           || c.unicode() == 0xb7;
}
} // namespace entities

// NOTE: must have prefix because `and`, `or`, and `int` are C++ keywords.
#define X(name, value) XID_##name = value
#define SEP_COMMA() ,

enum class NODISCARD XmlEntityEnum : uint16_t { INVALID = 0, X_FOREACH_ENTITY(X, SEP_COMMA) };

#undef X
#undef SEP_COMMA

struct NODISCARD XmlEntity final
{
    QByteArray short_name;
    QByteArray full_name;
    XmlEntityEnum id = XmlEntityEnum::INVALID;

    XmlEntity() = default;
    explicit XmlEntity(const char *const _short_name,
                       const char *const _full_name,
                       const XmlEntityEnum _id)
        : short_name{_short_name}
        , full_name{_full_name}
        , id{_id}
    {}
    DEFAULT_RULE_OF_5(XmlEntity);
};

using OptQByteArray = std::optional<QByteArray>;

struct NODISCARD EntityTable final
{
    struct NODISCARD MyHash final
    {
        NODISCARD uint32_t operator()(const QString &qs) const { return qHash(qs); }
    };

    std::unordered_map<QString, XmlEntity, MyHash> by_short_name;
    std::unordered_map<QString, XmlEntity, MyHash> by_full_name;
    std::unordered_map<XmlEntityEnum, XmlEntity> by_id;

    NODISCARD XmlEntityEnum lookup_entity_id_by_short_name(const QString &entity) const;
    NODISCARD XmlEntityEnum lookup_entity_id_by_full_name(const QString &entity) const;
    NODISCARD OptQByteArray lookup_entity_short_name_by_id(XmlEntityEnum id) const;
    NODISCARD OptQByteArray lookup_entity_full_name_by_id(XmlEntityEnum id) const;
};

NODISCARD static EntityTable initEntityTable()
{
    // prefixed enum class is an antipattern, but we may not be able to avoid it in this case.
#define X(name, value) (XmlEntity{#name, "&" #name ";", XmlEntityEnum::XID_##name})
#define SEP_COMMA() ,

    const std::vector<XmlEntity> all_entities{X_FOREACH_ENTITY(X, SEP_COMMA)};
    assert(all_entities.size() == 258);

#undef X
#undef SEP_COMMA

    EntityTable entityTable;
    for (const auto &ent : all_entities) {
        entityTable.by_short_name[QString::fromLatin1(ent.short_name)] = ent;
        entityTable.by_full_name[QString::fromLatin1(ent.full_name)] = ent;
        entityTable.by_id[ent.id] = ent;
    }

    return entityTable;
}

NODISCARD static const EntityTable &getEntityTable()
{
    static const auto g_entityTable = initEntityTable();
    return g_entityTable;
}

XmlEntityEnum EntityTable::lookup_entity_id_by_short_name(const QString &entity) const
{
    const auto &map = by_short_name;
    const auto it = map.find(entity);
    if (it != map.end()) {
        return it->second.id;
    }
    return XmlEntityEnum::INVALID;
}

XmlEntityEnum EntityTable::lookup_entity_id_by_full_name(const QString &entity) const
{
    const auto &map = by_full_name;
    const auto it = map.find(entity);
    if (it != map.end()) {
        return it->second.id;
    }
    return XmlEntityEnum::INVALID;
}

OptQByteArray EntityTable::lookup_entity_short_name_by_id(const XmlEntityEnum id) const
{
    const auto &map = by_id;
    const auto it = map.find(id);
    if (it != map.end()) {
        return OptQByteArray{it->second.short_name};
    }
    return std::nullopt;
}

OptQByteArray EntityTable::lookup_entity_full_name_by_id(const XmlEntityEnum id) const
{
    const auto &map = by_id;
    const auto it = map.find(id);
    if (it != map.end()) {
        return OptQByteArray{it->second.full_name};
    }
    return std::nullopt;
}

NODISCARD static const char *translit(const QChar qc)
{
    using namespace string_consts;

    // not fully implemented
    // note: some of these might be better off just giving the entity

    switch (static_cast<XmlEntityEnum>(qc.unicode())) {
    case XmlEntityEnum::XID_lang:
    case XmlEntityEnum::XID_laquo:
    case XmlEntityEnum::XID_lsaquo:
        return S_LESS_THAN;
    case XmlEntityEnum::XID_rang:
    case XmlEntityEnum::XID_raquo:
    case XmlEntityEnum::XID_rsaquo:
        return S_GREATER_THAN;

    case XmlEntityEnum::XID_loz:
        return "<>";

    case XmlEntityEnum::XID_larr:
        return "<-";
    case XmlEntityEnum::XID_harr:
        return "<->";
    case XmlEntityEnum::XID_rarr:
        return "->";

    case XmlEntityEnum::XID_lArr:
        return "<=";
    case XmlEntityEnum::XID_hArr:
        return "<=>";
    case XmlEntityEnum::XID_rArr:
        return "=>";

    case XmlEntityEnum::XID_thinsp:
    case XmlEntityEnum::XID_ensp:
    case XmlEntityEnum::XID_emsp:
        return S_SPACE;

    case XmlEntityEnum::XID_ndash:
    case XmlEntityEnum::XID_mdash:
        // case XmlEntityEnum::XID_oline:
        return S_MINUS_SIGN;

    case XmlEntityEnum::XID_horbar:
        return "--";

    case XmlEntityEnum::XID_lsquo:
    case XmlEntityEnum::XID_rsquo:
    case XmlEntityEnum::XID_prime:
        return S_SQUOTE;

    case XmlEntityEnum::XID_ldquo:
    case XmlEntityEnum::XID_rdquo:
    case XmlEntityEnum::XID_bdquo:
    case XmlEntityEnum::XID_Prime:
        return S_DQUOTE;

    case XmlEntityEnum::XID_frasl:
        return S_SLASH;

    case XmlEntityEnum::XID_sdot:
        return S_PERIOD;

    case XmlEntityEnum::XID_hellip:
        return "...";

    case XmlEntityEnum::XID_and:
    case XmlEntityEnum::XID_circ:
        return S_CARET;

    case XmlEntityEnum::XID_empty: {
        static_assert(static_cast<uint16_t>(XmlEntityEnum::XID_oslash) == 0xf8);
        return "\xf8";
    }

    case XmlEntityEnum::XID_lowast:
    case XmlEntityEnum::XID_bull:
        return S_ASTERISK;

    case XmlEntityEnum::XID_tilde:
    case XmlEntityEnum::XID_sim:
        return S_TILDE;

    case XmlEntityEnum::XID_ge:
        return ">=";
    case XmlEntityEnum::XID_le:
        return "<=";

    case XmlEntityEnum::XID_trade:
        return "TM";

    default:
        break;
    }

    return nullptr;
}

auto entities::encode(const DecodedUnicode &name, const EncodingEnum encodingType) -> EncodedLatin1
{
    const auto &tab = getEntityTable();

    EncodedLatin1 out;
    out.reserve(name.length());

    for (const QChar qc : name) {
        const auto codepoint = qc.unicode();
        if (codepoint < 256) {
            const char c = qc.toLatin1();
            switch (c) {
            case C_AMPERSAND:
                out += "&amp;";
                continue;
            case C_LESS_THAN:
                out += "&lt;";
                continue;
            case C_GREATER_THAN:
                out += "&gt;";
                continue;

            case C_NUL:
            case C_ALERT:
            case C_BACKSPACE:
            case C_FORM_FEED:
            case C_CARRIAGE_RETURN:
            case C_TAB:
            case C_VERTICAL_TAB:
            case C_NBSP:
                break;

            case C_NEWLINE:
            default:
                if (isprint(c) || isspace(c)) {
                    // REVISIT: transliterate unprintable latin1 code points here, or wait?
                    out += static_cast<char>(codepoint & 0xFF);
                    continue;
                }
            }
        }

        // first try transliteration
        if (encodingType == EncodingEnum::Translit) {
            if (const char *const subst = translit(qc)) {
                out += subst;
                continue;
            }
        }

        // then try named XML entities
        if (auto full_name = tab.lookup_entity_full_name_by_id(
                static_cast<XmlEntityEnum>(codepoint))) {
            out += full_name.value();
            continue;
        }

        /* this will be statically true until Qt changes the type to uint32_t */
        assert(static_cast<uint32_t>(codepoint) <= MAX_UNICODE_CODEPOINT);
        const auto masked = static_cast<int32_t>(codepoint & 0x1FFFFFu);
        assert(masked == codepoint);

        char decbuf[16];
        std::snprintf(decbuf, sizeof(decbuf), "&#%d;", masked);
        const size_t declen = strlen(decbuf);
        assert(declen <= 10);

        // finally, use hex-encoding "&x0100;" to "&x10FFFF;"
        char hexbuf[16];
        std::snprintf(hexbuf, sizeof(hexbuf), "&#x%X;", masked);
        const size_t hexlen = strlen(hexbuf);
        assert(hexlen <= 10);

        out += (hexlen <= declen) ? hexbuf : decbuf;

        // continue;
    }

    return out;
}

NODISCARD static OptQChar tryParseDec(const QChar *const beg, const QChar *const end)
{
    if (beg >= end || !isLatin1Digit(*beg)) {
        return OptQChar{};
    }

    using val_type = uint32_t;
    static_assert(std::numeric_limits<val_type>::max()
                  >= static_cast<uint64_t>(MAX_UNICODE_CODEPOINT) * 10 + 9);

    val_type val = 0;
    for (const QChar *it = beg; it < end; ++it) {
        const QChar qc = *it;
        if (!isLatin1(qc)) {
            return OptQChar{};
        }
        const char c = qc.toLatin1();
        if (!isdigit(c)) {
            return OptQChar{};
        }

        val *= 10;
        val += static_cast<uint32_t>(c - '0');

        if (val > MAX_UNICODE_CODEPOINT) {
            return OptQChar{};
        }
    }

    return OptQChar{val};
}

NODISCARD static OptQChar tryParseHex(const QChar *const beg, const QChar *const end)
{
    if (beg >= end || !isLatin1HexDigit(*beg)) {
        return OptQChar{};
    }

    using val_type = uint32_t;
    static_assert(std::numeric_limits<val_type>::max()
                  >= static_cast<uint64_t>(MAX_UNICODE_CODEPOINT) * 16 + 15);

    val_type val = 0;
    for (const QChar *it = beg; it < end; ++it) {
        const QChar qc = *it;
        if (!isLatin1(qc)) {
            return OptQChar{};
        }
        const char c = qc.toLatin1();
        if (!isxdigit(c)) {
            return OptQChar{};
        }

        val *= 16;
        val += isdigit(c) ? (static_cast<uint32_t>(c - '0'))
                          : (static_cast<uint32_t>(c - (isupper(c) ? 'A' : 'a')) + 10);

        if (val > MAX_UNICODE_CODEPOINT) {
            return OptQChar{};
        }
    }
    return OptQChar{val};
}

// TODO: test with strings like "", "&;", "&&", "&&;", "&lt", "&lt;" "&lt&lt;",
// "&foo;", "&1114111;", "&1114112;", "&x10FFFF;", "&x110000;", etc
void entities::foreachEntity(const QStringView input, EntityCallback &callback)
{
    const auto &tab = getEntityTable();

    const QChar *const beg = input.begin();
    const QChar *const end = input.end();

    for (const QChar *it = beg; it < end;) {
        if (*it != C_AMPERSAND) {
            // out.append(*it);
            ++it;
            continue;
        }

        const QChar *const amp = it++;
        const auto ampStart = static_cast<int>(amp - beg);
        if (it + 1 < end && *it == C_POUND_SIGN) {
            ++it;

            if (*it == 'x') {
                ++it;
                for (; it < end; ++it) {
                    if (!isLatin1HexDigit(*it)) {
                        break;
                    }
                }

                if (it < end && *it == C_SEMICOLON) {
                    ++it;

                    callback.decodedEntity(ampStart,
                                           static_cast<int>(it - amp),
                                           tryParseHex(amp + 3, it - 1));
                    continue;
                }

            } else if (isLatin1Digit(*it)) {
                for (; it < end; ++it) {
                    if (!isLatin1Digit(*it)) {
                        break;
                    }
                }

                if (it < end && *it == C_SEMICOLON) {
                    ++it;
                    callback.decodedEntity(ampStart,
                                           static_cast<int>(it - amp),
                                           tryParseDec(amp + 2, it - 1));
                    continue;
                }
            }
        } else if (isNameStartChar(*it)) {
            ++it;
            for (; it < end; ++it) {
                if (!isNameChar(*it)) {
                    break;
                }
            }

            if (it < end && *it == C_SEMICOLON) {
                ++it;
                const auto ampLen = static_cast<int>(it - amp);
                const auto full = input.mid(ampStart, ampLen).toString();
                assert(full.size() == ampLen);
                const XmlEntityEnum id = tab.lookup_entity_id_by_full_name(full);
                if (id != XmlEntityEnum::INVALID) {
                    const auto bits = static_cast<uint16_t>(id);
                    const QChar qc(bits);
                    assert(qc.unicode() == bits);
                    callback.decodedEntity(ampStart, ampLen, OptQChar{qc});
                    continue;
                } else {
                    callback.decodedEntity(ampStart, ampLen, OptQChar{});
                }
            }
        }

        // "&bogus" or "&bogus;"
        assert(beg <= amp && amp < it && it <= end);
        // for (const char *tmp = amp; tmp != it; ++tmp)
        //   out.append(*tmp);
        continue;
    }
}

auto entities::decode(const EncodedLatin1 &input) -> DecodedUnicode
{
    static constexpr const char unprintable = C_QUESTION_MARK;
    struct NODISCARD MyEntityCallback final : public EntityCallback
    {
    public:
        const EncodedLatin1 &input;
        DecodedUnicode out;
        int pos = 0;

    public:
        explicit MyEntityCallback(const EncodedLatin1 &_input)
            : input{_input}
        {
            out.reserve(input.size());
        }

    private:
        void virt_decodedEntity(int start, int len, OptQChar decoded) final
        {
            skipto(start);
            if (decoded) {
                out += decoded.value();
            } else {
                out += unprintable;
            }
            pos = start + len;
        }

    public:
        void skipto(int start)
        {
            assert(start >= this->pos);
            if (start == this->pos) {
                return;
            }

            const char *const pos_it = input.begin() + pos;
            const char *const start_it = input.begin() + start;

            for (const char *it = pos_it; it != start_it; ++it) {
                out += *it;
            }
            pos = start;
        }
    } callback{input};

    const QString &tmp = QString(input);
    assert(tmp.size() == input.size());

    foreachEntity(QStringView{tmp}, callback);
    callback.skipto(input.size());

    return std::move(callback.out);
}

// self test
namespace entities {
static void testEncode(const char *_in, const char *_expect)
{
    const auto in = DecodedUnicode{_in};
    const auto out = encode(in);
    const auto expected = EncodedLatin1{_expect};
    if (out != expected) {
        throw std::runtime_error("test failed");
    }
}
static void testDecode(const char *_in, const char *_expect)
{
    const auto in = EncodedLatin1{_in};
    const auto out = decode(in);
    const auto expected = DecodedUnicode{_expect};
    if (out != expected) {
        throw std::runtime_error("test failed");
    }
}

static const bool self_test = []() -> bool {
    using namespace string_consts;

    //
    testDecode("", "");
    testDecode("&amp;", S_AMPERSAND);
    testDecode("&nbsp;", S_NBSP);

    testDecode("&#9;", S_TAB);
    testDecode("&#x9;", S_TAB);
    testDecode("&#10;", S_NEWLINE);
    testDecode("&#xA;", S_NEWLINE);
    testDecode("&#x20;", S_SPACE);
    testDecode("&#32;", S_SPACE);
    testDecode("&#xFF;", "\xFF");
    testDecode("&#255;", "\xFF");

    //
    testEncode("", "");
    testEncode("&amp;", "&amp;amp;");
    testEncode(S_TAB, "&#9;");        // Note: chooses dec (#9) over hex (#x9).
    testEncode(S_NEWLINE, S_NEWLINE); // Note: chooses literal instead of dec (#10) or hex (#xA).
    testEncode("\x0B", "&#xB;");      // Note: chooses hex (#xB) over decimal (#11).
    {
        QString in;
        in += QChar{static_cast<uint16_t>(XmlEntityEnum::XID_trade)};
        const auto out = encode(DecodedUnicode{in});
        const auto expected = EncodedLatin1{"TM"};
        if (out != expected) {
            throw std::runtime_error("test failed");
        }
    }
    {
        QString in;
        in += QChar{static_cast<uint16_t>(XmlEntityEnum::XID_trade)};
        const auto out = encode(DecodedUnicode{in}, EncodingEnum::Lossless);
        const auto expected = EncodedLatin1{"&trade;"};
        if (out != expected) {
            throw std::runtime_error("test failed");
        }
    }
    {
        const auto in = EncodedLatin1{"&#xFFFF;"};
        const auto out = decode(in);
        assert(out.length() == 1);
        assert(out.at(0).unicode() == 0xFFFF);
        const auto roundtrip = encode(out);
        if (roundtrip != in) {
            throw std::runtime_error("test failed");
        }
    }

    {
        // Demonstration that values above U+FFFF are mangled.
        const auto in = EncodedLatin1{"&#x10FFFF;"};
        const auto out = decode(in);
        assert(out.length() == 1);
        assert(out.at(0).unicode() == 0xFFFF); // wrong since QT only stores 16 bits
        const auto roundtrip = encode(out);
        if (roundtrip != EncodedLatin1{"&#xFFFF;"}) { // also wrong, but expected.
            throw std::runtime_error("test failed");
        }
    }

    return true;
}();
} // namespace entities
