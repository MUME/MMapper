/************************************************************************
**
** Authors:   Nils Schimmelmann <nschimme@gmail.com>
**
** This file is part of the MMapper project.
** Maintained by Nils Schimmelmann <nschimme@gmail.com>
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the:
** Free Software Foundation, Inc.
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
************************************************************************/

#include "entities.h"

#include <cassert>
#include <cstdint>
#include <initializer_list>
#include <unordered_map>
#include <QByteArray>
#include <QHash>
#include <QString>

#include "RuleOf5.h"

static bool isLatin1(const QChar &qc)
{
    return qc.unicode() < 256;
}

static bool isLatin1Digit(const QChar &qc)
{
    return isLatin1(qc) && isdigit(qc.toLatin1());
}

static bool isLatin1HexDigit(const QChar &qc)
{
    return isLatin1(qc) && isxdigit(qc.toLatin1());
}

static bool isLatin1Alpha(const QChar &qc)
{
    return isLatin1(qc) && isalpha(qc.toLatin1());
}

entities::EntityCallback::~EntityCallback() = default;

// clang-format off
// https://en.wikipedia.org/wiki/List_of_XML_and_HTML_character_entity_references#Character_entity_references_in_HTML
#define X_FOREACH_ENTITY(X, XSEP) \
    X(excl, 0x0021) XSEP() \
    X(quot, 0x0022) XSEP() \
    X(percent, 0x0025) XSEP() \
    X(amp, 0x0026) XSEP() \
    X(apos, 0x0027) XSEP() \
    X(add, 0x002B) XSEP() \
    X(lt, 0x003C) XSEP() \
    X(equal, 0x003D) XSEP() \
    X(gt, 0x003E) XSEP() \
    X(nbsp, 0x00A0) XSEP() \
    X(iexcl, 0x00A1) XSEP() \
    X(cent, 0x00A2) XSEP() \
    X(pound, 0x00A3) XSEP() \
    X(curren, 0x00A4) XSEP() \
    X(yen, 0x00A5) XSEP() \
    X(brvbar, 0x00A6) XSEP() \
    X(sect, 0x00A7) XSEP() \
    X(uml, 0x00A8) XSEP() \
    X(copy, 0x00A9) XSEP() \
    X(ordf, 0x00AA) XSEP() \
    X(laquo, 0x00AB) XSEP() \
    X(not, 0x00AC) XSEP() \
    X(shy, 0x00AD) XSEP() \
    X(reg, 0x00AE) XSEP() \
    X(macr, 0x00AF) XSEP() \
    X(deg, 0x00B0) XSEP() \
    X(plusmn, 0x00B1) XSEP() \
    X(sup2, 0x00B2) XSEP() \
    X(sup3, 0x00B3) XSEP() \
    X(acute, 0x00B4) XSEP() \
    X(micro, 0x00B5) XSEP() \
    X(para, 0x00B6) XSEP() \
    X(middot, 0x00B7) XSEP() \
    X(cedil, 0x00B8) XSEP() \
    X(sup1, 0x00B9) XSEP() \
    X(ordm, 0x00BA) XSEP() \
    X(raquo, 0x00BB) XSEP() \
    X(frac14, 0x00BC) XSEP() \
    X(frac12, 0x00BD) XSEP() \
    X(frac34, 0x00BE) XSEP() \
    X(iquest, 0x00BF) XSEP() \
    X(Agrave, 0x00C0) XSEP() \
    X(Aacute, 0x00C1) XSEP() \
    X(Acirc, 0x00C2) XSEP() \
    X(Atilde, 0x00C3) XSEP() \
    X(Auml, 0x00C4) XSEP() \
    X(Aring, 0x00C5) XSEP() \
    X(AElig, 0x00C6) XSEP() \
    X(Ccedil, 0x00C7) XSEP() \
    X(Egrave, 0x00C8) XSEP() \
    X(Eacute, 0x00C9) XSEP() \
    X(Ecirc, 0x00CA) XSEP() \
    X(Euml, 0x00CB) XSEP() \
    X(Igrave, 0x00CC) XSEP() \
    X(Iacute, 0x00CD) XSEP() \
    X(Icirc, 0x00CE) XSEP() \
    X(Iuml, 0x00CF) XSEP() \
    X(ETH, 0x00D0) XSEP() \
    X(Ntilde, 0x00D1) XSEP() \
    X(Ograve, 0x00D2) XSEP() \
    X(Oacute, 0x00D3) XSEP() \
    X(Ocirc, 0x00D4) XSEP() \
    X(Otilde, 0x00D5) XSEP() \
    X(Ouml, 0x00D6) XSEP() \
    X(times, 0x00D7) XSEP() \
    X(Oslash, 0x00D8) XSEP() \
    X(Ugrave, 0x00D9) XSEP() \
    X(Uacute, 0x00DA) XSEP() \
    X(Ucirc, 0x00DB) XSEP() \
    X(Uuml, 0x00DC) XSEP() \
    X(Yacute, 0x00DD) XSEP() \
    X(THORN, 0x00DE) XSEP() \
    X(szlig, 0x00DF) XSEP() \
    X(agrave, 0x00E0) XSEP() \
    X(aacute, 0x00E1) XSEP() \
    X(acirc, 0x00E2) XSEP() \
    X(atilde, 0x00E3) XSEP() \
    X(auml, 0x00E4) XSEP() \
    X(aring, 0x00E5) XSEP() \
    X(aelig, 0x00E6) XSEP() \
    X(ccedil, 0x00E7) XSEP() \
    X(egrave, 0x00E8) XSEP() \
    X(eacute, 0x00E9) XSEP() \
    X(ecirc, 0x00EA) XSEP() \
    X(euml, 0x00EB) XSEP() \
    X(igrave, 0x00EC) XSEP() \
    X(iacute, 0x00ED) XSEP() \
    X(icirc, 0x00EE) XSEP() \
    X(iuml, 0x00EF) XSEP() \
    X(eth, 0x00F0) XSEP() \
    X(ntilde, 0x00F1) XSEP() \
    X(ograve, 0x00F2) XSEP() \
    X(oacute, 0x00F3) XSEP() \
    X(ocirc, 0x00F4) XSEP() \
    X(otilde, 0x00F5) XSEP() \
    X(ouml, 0x00F6) XSEP() \
    X(divide, 0x00F7) XSEP() \
    X(oslash, 0x00F8) XSEP() \
    X(ugrave, 0x00F9) XSEP() \
    X(uacute, 0x00FA) XSEP() \
    X(ucirc, 0x00FB) XSEP() \
    X(uuml, 0x00FC) XSEP() \
    X(yacute, 0x00FD) XSEP() \
    X(thorn, 0x00FE) XSEP() \
    X(yuml, 0x00FF) XSEP() \
    X(OElig, 0x0152) XSEP() \
    X(oelig, 0x0153) XSEP() \
    X(Scaron, 0x0160) XSEP() \
    X(scaron, 0x0161) XSEP() \
    X(Yuml, 0x0178) XSEP() \
    X(fnof, 0x0192) XSEP() \
    X(circ, 0x02C6) XSEP() \
    X(tilde, 0x02DC) XSEP() \
    X(Alpha, 0x0391) XSEP() \
    X(Beta, 0x0392) XSEP() \
    X(Gamma, 0x0393) XSEP() \
    X(Delta, 0x0394) XSEP() \
    X(Epsilon, 0x0395) XSEP() \
    X(Zeta, 0x0396) XSEP() \
    X(Eta, 0x0397) XSEP() \
    X(Theta, 0x0398) XSEP() \
    X(Iota, 0x0399) XSEP() \
    X(Kappa, 0x039A) XSEP() \
    X(Lambda, 0x039B) XSEP() \
    X(Mu, 0x039C) XSEP() \
    X(Nu, 0x039D) XSEP() \
    X(Xi, 0x039E) XSEP() \
    X(Omicron, 0x039F) XSEP() \
    X(Pi, 0x03A0) XSEP() \
    X(Rho, 0x03A1) XSEP() \
    X(Sigma, 0x03A3) XSEP() \
    X(Tau, 0x03A4) XSEP() \
    X(Upsilon, 0x03A5) XSEP() \
    X(Phi, 0x03A6) XSEP() \
    X(Chi, 0x03A7) XSEP() \
    X(Psi, 0x03A8) XSEP() \
    X(Omega, 0x03A9) XSEP() \
    X(alpha, 0x03B1) XSEP() \
    X(beta, 0x03B2) XSEP() \
    X(gamma, 0x03B3) XSEP() \
    X(delta, 0x03B4) XSEP() \
    X(epsilon, 0x03B5) XSEP() \
    X(zeta, 0x03B6) XSEP() \
    X(eta, 0x03B7) XSEP() \
    X(theta, 0x03B8) XSEP() \
    X(iota, 0x03B9) XSEP() \
    X(kappa, 0x03BA) XSEP() \
    X(lambda, 0x03BB) XSEP() \
    X(mu, 0x03BC) XSEP() \
    X(nu, 0x03BD) XSEP() \
    X(xi, 0x03BE) XSEP() \
    X(omicron, 0x03BF) XSEP() \
    X(pi, 0x03C0) XSEP() \
    X(rho, 0x03C1) XSEP() \
    X(sigmaf, 0x03C2) XSEP() \
    X(sigma, 0x03C3) XSEP() \
    X(tau, 0x03C4) XSEP() \
    X(upsilon, 0x03C5) XSEP() \
    X(phi, 0x03C6) XSEP() \
    X(chi, 0x03C7) XSEP() \
    X(psi, 0x03C8) XSEP() \
    X(omega, 0x03C9) XSEP() \
    X(thetasym, 0x03D1) XSEP() \
    X(upsih, 0x03D2) XSEP() \
    X(piv, 0x03D6) XSEP() \
    X(ensp, 0x2002) XSEP() \
    X(emsp, 0x2003) XSEP() \
    X(thinsp, 0x2009) XSEP() \
    X(zwnj, 0x200C) XSEP() \
    X(zwj, 0x200D) XSEP() \
    X(lrm, 0x200E) XSEP() \
    X(rlm, 0x200F) XSEP() \
    X(ndash, 0x2013) XSEP() \
    X(mdash, 0x2014) XSEP() \
    X(horbar, 0x2015) XSEP() \
    X(lsquo, 0x2018) XSEP() \
    X(rsquo, 0x2019) XSEP() \
    X(sbquo, 0x201A) XSEP() \
    X(ldquo, 0x201C) XSEP() \
    X(rdquo, 0x201D) XSEP() \
    X(bdquo, 0x201E) XSEP() \
    X(dagger, 0x2020) XSEP() \
    X(Dagger, 0x2021) XSEP() \
    X(bull, 0x2022) XSEP() \
    X(hellip, 0x2026) XSEP() \
    X(permil, 0x2030) XSEP() \
    X(prime, 0x2032) XSEP() \
    X(Prime, 0x2033) XSEP() \
    X(lsaquo, 0x2039) XSEP() \
    X(rsaquo, 0x203A) XSEP() \
    X(oline, 0x203E) XSEP() \
    X(frasl, 0x2044) XSEP() \
    X(euro, 0x20AC) XSEP() \
    X(image, 0x2111) XSEP() \
    X(weierp, 0x2118) XSEP() \
    X(real, 0x211C) XSEP() \
    X(trade, 0x2122) XSEP() \
    X(alefsym, 0x2135) XSEP() \
    X(larr, 0x2190) XSEP() \
    X(uarr, 0x2191) XSEP() \
    X(rarr, 0x2192) XSEP() \
    X(darr, 0x2193) XSEP() \
    X(harr, 0x2194) XSEP() \
    X(crarr, 0x21B5) XSEP() \
    X(lArr, 0x21D0) XSEP() \
    X(uArr, 0x21D1) XSEP() \
    X(rArr, 0x21D2) XSEP() \
    X(dArr, 0x21D3) XSEP() \
    X(hArr, 0x21D4) XSEP() \
    X(forall, 0x2200) XSEP() \
    X(part, 0x2202) XSEP() \
    X(exist, 0x2203) XSEP() \
    X(empty, 0x2205) XSEP() \
    X(nabla, 0x2207) XSEP() \
    X(isin, 0x2208) XSEP() \
    X(notin, 0x2209) XSEP() \
    X(ni, 0x220B) XSEP() \
    X(prod, 0x220F) XSEP() \
    X(sum, 0x2211) XSEP() \
    X(minus, 0x2212) XSEP() \
    X(lowast, 0x2217) XSEP() \
    X(radic, 0x221A) XSEP() \
    X(prop, 0x221D) XSEP() \
    X(infin, 0x221E) XSEP() \
    X(ang, 0x2220) XSEP() \
    X(and, 0x2227) XSEP() \
    X(or, 0x2228) XSEP() \
    X(cap, 0x2229) XSEP() \
    X(cup, 0x222A) XSEP() \
    X(int, 0x222B) XSEP() \
    X(there4, 0x2234) XSEP() \
    X(sim, 0x223C) XSEP() \
    X(cong, 0x2245) XSEP() \
    X(asymp, 0x2248) XSEP() \
    X(ne, 0x2260) XSEP() \
    X(equiv, 0x2261) XSEP() \
    X(le, 0x2264) XSEP() \
    X(ge, 0x2265) XSEP() \
    X(sub, 0x2282) XSEP() \
    X(sup, 0x2283) XSEP() \
    X(nsub, 0x2284) XSEP() \
    X(sube, 0x2286) XSEP() \
    X(supe, 0x2287) XSEP() \
    X(oplus, 0x2295) XSEP() \
    X(otimes, 0x2297) XSEP() \
    X(perp, 0x22A5) XSEP() \
    X(sdot, 0x22C5) XSEP() \
    X(lceil, 0x2308) XSEP() \
    X(rceil, 0x2309) XSEP() \
    X(lfloor, 0x230A) XSEP() \
    X(rfloor, 0x230B) XSEP() \
    X(lang, 0x2329) XSEP() \
    X(rang, 0x232A) XSEP() \
    X(loz, 0x25CA) XSEP() \
    X(spades, 0x2660) XSEP() \
    X(clubs, 0x2663) XSEP() \
    X(hearts, 0x2665) XSEP() \
    X(diams, 0x2666)
// clang-format on

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
static bool isNameStartChar(const QChar c)
{
    if (c == ':' || c == '_' || isLatin1Alpha(c))
        return true;
    const auto uc = static_cast<uint32_t>(c.unicode());
    return uc >= 0xc0 && uc != 0xd7 && uc != 0xf7;
}

// only bother with the latin1 subset
static bool isNameChar(const QChar c)
{
    return isNameStartChar(c) || c == '-' || c == '.' || isLatin1Digit(c) || c.unicode() == 0xb7;
}
} // namespace entities

// NOTE: must have prefix because `and`, `or`, and `int` are C++ keywords.
#define X(name, value) XID_##name = value
#define SEP_COMMA() ,

enum class XmlEntityId : uint16_t { INVALID = 0, X_FOREACH_ENTITY(X, SEP_COMMA) };

#undef X
#undef SEP_COMMA

struct XmlEntity final
{
    QByteArray short_name;
    QByteArray full_name;
    XmlEntityId id = XmlEntityId::INVALID;

    explicit XmlEntity() = default;
    explicit XmlEntity(const char *const _short_name,
                       const char *const _full_name,
                       const XmlEntityId _id)
        : short_name{_short_name}
        , full_name{_full_name}
        , id{_id}
    {}
    DEFAULT_RULE_OF_5(XmlEntity);
};

struct OptQByteArray
{
    QByteArray arr;
    bool valid = false;

    explicit OptQByteArray() = default;
    explicit OptQByteArray(QByteArray _arr)
        : arr{_arr}
        , valid{true}
    {}

    explicit operator bool() const { return valid; }

    void value() && = delete;
    void value() const && = delete;

    QByteArray &value() & { return arr; }
    const QByteArray &value() const & { return arr; }
};

struct EntityTable final
{
    struct MyHash
    {
        uint32_t operator()(const QString &qs) const { return qHash(qs); }
    };

    // REVISIT (perf/alloc): provide hash and equality template parameters?
    std::unordered_map<QString, XmlEntity, MyHash> by_short_name;
    std::unordered_map<QString, XmlEntity, MyHash> by_full_name;
    std::unordered_map<XmlEntityId, XmlEntity> by_id;

    XmlEntityId lookup_entity_id_by_short_name(const QString &entity) const;
    XmlEntityId lookup_entity_id_by_full_name(const QString &entity) const;
    OptQByteArray lookup_entity_short_name_by_id(const XmlEntityId id) const;
    OptQByteArray lookup_entity_full_name_by_id(const XmlEntityId id) const;
};

static EntityTable initEntityTable()
{
#define X(name, value) \
    XmlEntity { #name, "&" #name ";", XmlEntityId::XID_##name }
#define SEP_COMMA() ,

    const std::initializer_list<XmlEntity> all_entities{X_FOREACH_ENTITY(X, SEP_COMMA)};
    assert(all_entities.size() == 258);

#undef X
#undef SEP_COMMA

    EntityTable entityTable;
    for (auto &ent : all_entities) {
        entityTable.by_short_name[QString::fromLatin1(ent.short_name)] = ent;
        entityTable.by_full_name[QString::fromLatin1(ent.full_name)] = ent;
        entityTable.by_id[ent.id] = ent;
    }

    return entityTable;
}

static const EntityTable &getEntityTable()
{
    static const auto g_entityTable = initEntityTable();
    return g_entityTable;
}

XmlEntityId EntityTable::lookup_entity_id_by_short_name(const QString &entity) const
{
    const auto &map = by_short_name;
    const auto it = map.find(entity);
    if (it != map.end())
        return it->second.id;
    return XmlEntityId::INVALID;
}

XmlEntityId EntityTable::lookup_entity_id_by_full_name(const QString &entity) const
{
    const auto &map = by_full_name;
    const auto it = map.find(entity);
    if (it != map.end())
        return it->second.id;
    return XmlEntityId::INVALID;
}

OptQByteArray EntityTable::lookup_entity_short_name_by_id(const XmlEntityId id) const
{
    const auto &map = by_id;
    const auto it = map.find(id);
    if (it != map.end())
        return OptQByteArray{it->second.short_name};
    return OptQByteArray{};
}

OptQByteArray EntityTable::lookup_entity_full_name_by_id(const XmlEntityId id) const
{
    const auto &map = by_id;
    const auto it = map.find(id);
    if (it != map.end())
        return OptQByteArray{it->second.full_name};
    return OptQByteArray{};
}

static const char *translit(const QChar qc)
{
    // not fully implemented
    // note: some of these might be better off just giving the entity

    switch (static_cast<XmlEntityId>(qc.unicode())) {
    case XmlEntityId::XID_lang:
    case XmlEntityId::XID_laquo:
    case XmlEntityId::XID_lsaquo:
        return "<";
    case XmlEntityId::XID_rang:
    case XmlEntityId::XID_raquo:
    case XmlEntityId::XID_rsaquo:
        return ">";

    case XmlEntityId::XID_loz:
        return "<>";

    case XmlEntityId::XID_larr:
        return "<-";
    case XmlEntityId::XID_harr:
        return "<->";
    case XmlEntityId::XID_rarr:
        return "->";

    case XmlEntityId::XID_lArr:
        return "<=";
    case XmlEntityId::XID_hArr:
        return "<=>";
    case XmlEntityId::XID_rArr:
        return "=>";

    case XmlEntityId::XID_thinsp:
    case XmlEntityId::XID_ensp:
    case XmlEntityId::XID_emsp:
        return " ";

    case XmlEntityId::XID_ndash:
    case XmlEntityId::XID_mdash:
        // case XmlEntityId::XID_oline:
        return "-";

    case XmlEntityId::XID_horbar:
        return "--";

    case XmlEntityId::XID_lsquo:
    case XmlEntityId::XID_rsquo:
    case XmlEntityId::XID_prime:
        return "\'";

    case XmlEntityId::XID_ldquo:
    case XmlEntityId::XID_rdquo:
    case XmlEntityId::XID_bdquo:
    case XmlEntityId::XID_Prime:
        return "\"";

    case XmlEntityId::XID_frasl:
        return "/";

    case XmlEntityId::XID_sdot:
        return ".";

    case XmlEntityId::XID_hellip:
        return "...";

    case XmlEntityId::XID_and:
    case XmlEntityId::XID_circ:
        return "^";

    case XmlEntityId::XID_empty: {
        static_assert(static_cast<uint16_t>(XmlEntityId::XID_oslash) == 0xf8, "");
        return "\xf8";
    }

    case XmlEntityId::XID_lowast:
    case XmlEntityId::XID_bull:
        return "*";

    case XmlEntityId::XID_tilde:
    case XmlEntityId::XID_sim:
        return "~";

    case XmlEntityId::XID_ge:
        return ">=";
    case XmlEntityId::XID_le:
        return "<=";

    case XmlEntityId::XID_trade:
        return "TM";

    default:
        break;
    }

    return nullptr;
}

auto entities::encode(const DecodedUnicode &name, const EncodingType encodingType) -> EncodedLatin1
{
    const auto &tab = getEntityTable();

    EncodedLatin1 out;
    out.reserve(name.length());

    for (const QChar &qc : name) {
        const auto codepoint = qc.unicode();
        if (codepoint < 256) {
            const char c = qc.toLatin1();
            switch (c) {
            case '&':
                out += "&amp;";
                continue;
            case '<':
                out += "&lt;";
                continue;
            case '>':
                out += "&gt;";
                continue;

            case '\0':
            case '\a':
            case '\b':
            case '\f':
            case '\r':
            case '\t':
            case '\v':
            case '\xa0':
                break;

            case '\n':
            default:
                if (isprint(c) || isspace(c)) {
                    // REVISIT: transliterate unprintable latin1 code points here, or wait?
                    out += static_cast<char>(codepoint & 0xFF);
                    continue;
                }
            }
        }

        // first try transliteration
        if (encodingType == EncodingType::Translit)
            if (const char *const subst = translit(qc)) {
                out += subst;
                continue;
            }

        // then try named XML entities
        if (auto full_name = tab.lookup_entity_full_name_by_id(
                static_cast<XmlEntityId>(codepoint))) {
            out += full_name.value();
            continue;
        }

        /* this will be statically true until Qt changes the type to uint32_t */
        assert(static_cast<uint32_t>(codepoint) <= MAX_UNICODE_CODEPOINT);
        const int masked = codepoint & 0x1FFFFFu;
        assert(masked == codepoint);

        char decbuf[16];
        sprintf(decbuf, "&#%d;", masked);
        const size_t declen = strlen(decbuf);
        assert(declen <= 10);

        // finally, use hex-encoding "&x0100;" to "&x10FFFF;"
        char hexbuf[16];
        sprintf(hexbuf, "&#x%X;", masked);
        const size_t hexlen = strlen(hexbuf);
        assert(hexlen <= 10);

        out += (hexlen <= declen) ? hexbuf : decbuf;

        continue;
    }

    return out;
}

static OptQChar tryParseDec(const QChar *const beg, const QChar *const end)
{
    if (beg >= end || !isLatin1Digit(*beg))
        return OptQChar{};

    using val_type = uint32_t;
    static_assert(std::numeric_limits<val_type>::max()
                      >= static_cast<uint64_t>(MAX_UNICODE_CODEPOINT) * 10 + 9,
                  "");

    val_type val = 0;
    for (const QChar *it = beg; it < end; ++it) {
        const QChar &qc = *it;
        if (!isLatin1(qc))
            return OptQChar{};
        const char c = qc.toLatin1();
        if (!isdigit(c))
            return OptQChar{};

        val *= 10;
        val += static_cast<uint32_t>(c - '0');

        if (val > MAX_UNICODE_CODEPOINT)
            return OptQChar{};
    }

    return OptQChar{val};
}

static OptQChar tryParseHex(const QChar *const beg, const QChar *const end)
{
    if (beg >= end || !isLatin1HexDigit(*beg))
        return OptQChar{};

    using val_type = uint32_t;
    static_assert(std::numeric_limits<val_type>::max()
                      >= static_cast<uint64_t>(MAX_UNICODE_CODEPOINT) * 16 + 15,
                  "");

    val_type val = 0;
    for (const QChar *it = beg; it < end; ++it) {
        const QChar &qc = *it;
        if (!isLatin1(qc))
            return OptQChar{};
        const char c = qc.toLatin1();
        if (!isxdigit(c))
            return OptQChar{};

        val *= 16;
        val += isdigit(c) ? (static_cast<uint32_t>(c - '0'))
                          : (static_cast<uint32_t>(c - (isupper(c) ? 'A' : 'a')) + 10);

        if (val > MAX_UNICODE_CODEPOINT)
            return OptQChar{};
    }
    return OptQChar{val};
}

// TODO: test with strings like "", "&;", "&&", "&&;", "&lt", "&lt;" "&lt&lt;",
// "&foo;", "&1114111;", "&1114112;", "&x10FFFF;", "&x110000;", etc
void entities::foreachEntity(const QStringRef &input, EntityCallback &callback)
{
    const auto &tab = getEntityTable();

    const QChar *const beg = input.begin();
    const QChar *const end = input.end();

    for (const QChar *it = beg; it < end;) {
        if (*it != '&') {
            // out.append(*it);
            ++it;
            continue;
        }

        const QChar *const amp = it++;
        const auto ampStart = static_cast<int>(amp - beg);
        if (it + 1 < end && *it == '#') {
            ++it;

            if (*it == 'x') {
                ++it;
                for (; it < end; ++it)
                    if (!isLatin1HexDigit(*it))
                        break;

                if (it < end && *it == ';') {
                    ++it;

                    callback.decodedEntity(ampStart, static_cast<int>(it - amp), tryParseHex(amp + 3, it - 1));
                    continue;
                }

            } else if (isLatin1Digit(*it)) {
                for (; it < end; ++it)
                    if (!isLatin1Digit(*it))
                        break;

                if (it < end && *it == ';') {
                    ++it;
                    callback.decodedEntity(ampStart, static_cast<int>(it - amp), tryParseDec(amp + 2, it - 1));
                    continue;
                }
            }
        } else if (isNameStartChar(*it)) {
            ++it;
            for (; it < end; ++it)
                if (!isNameChar(*it))
                    break;

            if (it < end && *it == ';') {
                ++it;
                const auto ampLen = static_cast<int>(it - amp);
                const auto full = input.mid(ampStart, ampLen).toString();
                assert(full.size() == ampLen);
                const XmlEntityId id = tab.lookup_entity_id_by_full_name(full);
                if (id != XmlEntityId::INVALID) {
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
    static constexpr const char unprintable = '?';
    struct MyEntityCallback final : EntityCallback
    {
        const EncodedLatin1 &input;
        DecodedUnicode out{};
        int pos = 0;

        explicit MyEntityCallback(const EncodedLatin1 &_input)
            : input{_input}
        {
            out.reserve(input.size());
        }

        virtual void decodedEntity(int start, int len, OptQChar decoded) override
        {
            skipto(start);
            if (decoded)
                out += decoded.value();
            else
                out += unprintable;
            pos = start + len;
        }

        void skipto(int start)
        {
            assert(start >= this->pos);
            if (start == this->pos)
                return;

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

    foreachEntity(tmp.midRef(0), callback);
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
    if (out != expected)
        throw std::runtime_error("test failed");
}
static void testDecode(const char *_in, const char *_expect)
{
    const auto in = EncodedLatin1{_in};
    const auto out = decode(in);
    const auto expected = DecodedUnicode{_expect};
    if (out != expected)
        throw std::runtime_error("test failed");
}

static bool self_test = []() -> bool {
    //
    testDecode("", "");
    testDecode("&amp;", "&");
    testDecode("&nbsp;", "\xa0");

    testDecode("&#9;", "\t");
    testDecode("&#x9;", "\t");
    testDecode("&#10;", "\n");
    testDecode("&#xA;", "\n");
    testDecode("&#x20;", " ");
    testDecode("&#32;", " ");
    testDecode("&#xFF;", "\xFF");
    testDecode("&#255;", "\xFF");

    //
    testEncode("", "");
    testEncode("&amp;", "&amp;amp;");
    testEncode("\t", "&#9;");    // Note: chooses dec (#9) over hex (#x9).
    testEncode("\n", "\n");      // Note: chooses literal instead of dec (#10) or hex (#xA).
    testEncode("\x0B", "&#xB;"); // Note: chooses hex (#xB) over decimal (#11).
    {
        QString in;
        in += QChar{static_cast<uint16_t>(XmlEntityId::XID_trade)};
        const auto out = encode(DecodedUnicode{in});
        const auto expected = EncodedLatin1{"TM"};
        if (out != expected)
            throw std::runtime_error("test failed");
    }
    {
        QString in;
        in += QChar{static_cast<uint16_t>(XmlEntityId::XID_trade)};
        const auto out = encode(DecodedUnicode{in}, EncodingType::Lossless);
        const auto expected = EncodedLatin1{"&trade;"};
        if (out != expected)
            throw std::runtime_error("test failed");
    }
    {
        const auto in = EncodedLatin1{"&#xFFFF;"};
        const auto out = decode(in);
        assert(out.length() == 1);
        assert(out.at(0).unicode() == 0xFFFF);
        const auto roundtrip = encode(out);
        if (roundtrip != in)
            throw std::runtime_error("test failed");
    }

    {
        // Demonstration that values above U+FFFF are mangled.
        const auto in = EncodedLatin1{"&#x10FFFF;"};
        const auto out = decode(in);
        assert(out.length() == 1);
        assert(out.at(0).unicode() == 0xFFFF); // wrong since QT only stores 16 bits
        const auto roundtrip = encode(out);
        if (roundtrip != EncodedLatin1{"&#xFFFF;"}) // also wrong, but expected.
            throw std::runtime_error("test failed");
    }

    return true;
}();
} // namespace entities
