// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "Diff.h"

#include "../global/AnsiOstream.h"
#include "../global/ConfigConsts.h"
#include "../global/Diff.h"
#include "../global/LineUtils.h"
#include "../global/StringView.h"
#include "../global/logging.h"
#include "ChangePrinter.h"
#include "InvalidMapOperation.h"
#include "RoomFields.h"
#include "World.h"

#include <ostream>
#include <set>
#include <sstream>
#include <vector>

namespace { // anonymous

template<typename T>
NODISCARD bool isDefault(const T &x)
{
    return x == T{};
}

NODISCARD bool isDefault(const RoomFieldVariant &var)
{
    bool result = false;
    var.acceptVisitor([&result](const auto &x) { result = isDefault(x); });
    return result;
}
NODISCARD bool isDefault(const ExitFieldVariant &var)
{
    bool result = false;
    var.acceptVisitor([&result](const auto &x) { result = isDefault(x); });
    return result;
}

void compare_exit(IDiffReporter &diff,
                  const RoomHandle &a,
                  const RoomHandle &b,
                  const ExitDirEnum dir)
{
    if (a.getId() != b.getId()) {
        throw InvalidMapOperation();
    }

    const RawExit &aex = a.getExit(dir);
    const RawExit &bex = b.getExit(dir);

    {
#define X_NOP()
#define X_DECL_VAR_TYPES(UPPER_CASE, CamelCase) \
    do { \
        const auto aval = ExitFieldVariant{aex.get##CamelCase()}; \
        const auto bval = ExitFieldVariant{bex.get##CamelCase()}; \
        const bool areSame = aval == bval; \
        if (!areSame) { \
            diff.exitFieldDifference(a, b, dir, aval, bval); \
        } \
    } while (false);
        XFOREACH_EXIT_FIELD(X_DECL_VAR_TYPES, X_NOP)
#undef X_DECL_VAR_TYPES
#undef X_NOP
    }

    const TinyRoomIdSet &aset = aex.getOutgoingSet();
    const TinyRoomIdSet &bset = bex.getOutgoingSet();
    const bool areSame = aset == bset;
    if (!areSame) {
        diff.exitOutgoingDifference(a, b, dir, aset, bset);
    }
}

NODISCARD std::string_view remove_punct_prefix(std::string_view &sv)
{
    return text_utils::take_prefix_matching(sv, ascii::isPunct);
}

NODISCARD std::string_view remove_punct_suffix(std::string_view &sv)
{
    return text_utils::take_suffix_matching(sv, ascii::isPunct);
}

NODISCARD std::vector<std::string_view> splitWordLines(const std::string_view s)
{
    using char_consts::C_NEWLINE;
    using char_consts::C_PERIOD;
    std::vector<std::string_view> wordsAndNewlines;

    auto insertPunct = [&wordsAndNewlines](std::string_view punct) {
        while (!punct.empty()) {
            if (punct.front() != C_PERIOD) {
                wordsAndNewlines.emplace_back(punct.substr(0, 1));
                punct.remove_prefix(1);
                continue;
            }

            const auto dots = punct.find_first_not_of(C_PERIOD);
            if (dots == std::string_view::npos) {
                wordsAndNewlines.emplace_back(punct);
                break;
            }

            wordsAndNewlines.emplace_back(punct.substr(0, dots));
            punct.remove_prefix(dots);
        }
    };

    auto insertWord = [&insertPunct, &wordsAndNewlines](std::string_view word) {
        assert(!word.empty());
        if (ascii::isPunct(word.front())) {
            auto punct = remove_punct_prefix(word);
            insertPunct(punct);
            if (word.empty()) {
                return;
            }
        }

        assert(!word.empty());
        if (!ascii::isPunct(word.back())) {
            // ignore possible internal punctuation like: foo's
            wordsAndNewlines.emplace_back(word);
            return;
        }

        auto punct = remove_punct_suffix(word);
        if (!word.empty()) {
            wordsAndNewlines.emplace_back(word);
        }
        insertPunct(punct);
    };

    auto insertWords = [&insertWord](std::string_view line) {
        assert(line.find(C_NEWLINE) == std::string_view::npos);
        StringView sv{line};
        sv.trim();
        while (!sv.empty()) {
            insertWord(sv.takeFirstWord().getStdStringView());
        }
    };

    foreachLine(s, [&insertWords, &wordsAndNewlines](std::string_view line) {
        if (line.empty()) {
            return;
        }

        if (line.back() != C_NEWLINE) {
            insertWords(line);
            return;
        }

        // REVISIT: do we need to check for carriage return?
        const auto newlinePos = line.size() - 1;
        insertWords(line.substr(0, newlinePos));
        wordsAndNewlines.emplace_back(line.substr(newlinePos, 1));
    });
    return wordsAndNewlines;
}

template<typename Tag>
void printDiff(AnsiOstream &os,
               const TaggedBoxedStringUtf8<Tag> &a,
               const TaggedBoxedStringUtf8<Tag> &b)
{
    // calls void printDiff(std::ostream &os, const std::string_view a, const std::string_view b);
    ::printDiff(os, a.getStdStringViewUtf8(), b.getStdStringViewUtf8());
}

} // namespace

void compare(IDiffReporter &diff, const RoomHandle &a, const RoomHandle &b)
{
    if (a.getIdExternal() != b.getIdExternal()) {
        throw InvalidMapOperation();
    }

    if (a.getServerId() != b.getServerId()) {
        diff.roomServerIdDifference(a, b);
    }
    if (a.getPosition() != b.getPosition()) {
        diff.roomPositionDifference(a, b);
    }
    if (a.isTemporary() != b.isTemporary()) {
        diff.roomStatusDifference(a, b);
    }

    // fields
    {
#define X_CMP_FIELD(_UPPER_CASE, _CamelCase, _Type) \
    do { \
        const auto aval = RoomFieldVariant{a.get##_CamelCase()}; \
        const auto bval = RoomFieldVariant{b.get##_CamelCase()}; \
        const bool areSame = aval == bval; \
        if (!areSame) { \
            diff.roomFieldDifference(a, b, aval, bval); \
        } \
    } while (false);
#define X_NOP()
        XFOREACH_ROOM_FIELD(X_CMP_FIELD, X_NOP)
        // XFOREACH_ROOM_PROPERTY(X_CMP_FIELD)
#undef X_NOP
#undef X_CMP_FIELD
    }

#undef COMPARE
    {
        for (const ExitDirEnum dir : ALL_EXITS7) {
            compare_exit(diff, a, b, dir);
        }
    }
}

void printDiff(AnsiOstream &os, const std::string_view a, const std::string_view b)
{
    assert(a != b);
    const auto aTokens = splitWordLines(a);
    const auto bTokens = splitWordLines(b);

    struct NODISCARD MyGetScore final
    {
        NODISCARD float operator()(const std::string_view a, const std::string_view b) const
        {
            if (a != b) {
                return 0.f;
            }

            if (a == string_consts::SV_NEWLINE) {
                return 0.1f;
            }

            const float scale = ascii::isPunct(a.front()) ? 0.05f : 1.f;

            // letters - words / 1000  = gently encourages longer matches
            return std::max(1e-4f, static_cast<float>(a.size()) * scale - 1e-3f);
        }
    };

    struct NODISCARD MyDiff final : ::diff::Diff<std::string_view, MyGetScore>
    {
    private:
        enum class NODISCARD LineStateEnum : uint8_t { Newline, OpenQuote, Text };

    private:
        AnsiOstream &m_os;
        LineStateEnum m_lineState = LineStateEnum::Newline;

    public:
        explicit MyDiff(AnsiOstream &os)
            : m_os{os}
        {
            openLine();
        }

        ~MyDiff() final
        {
            if (m_lineState != LineStateEnum::Newline) {
                closeLine();
            }
        }

    private:
        void printDQuote()
        {
            RawAnsi yellow;
            yellow.fg = AnsiColorVariant{AnsiColor16Enum::yellow};
            m_os.writeWithColor(yellow, string_consts::SV_DQUOTE);
        }

    private:
        void openLine()
        {
            assert(m_lineState == LineStateEnum::Newline);
            m_os << char_consts::C_AT_SIGN << char_consts::C_SPACE;
            printDQuote();
            m_lineState = LineStateEnum::OpenQuote;
        }
        void maybeSpace()
        {
            switch (m_lineState) {
            case LineStateEnum::OpenQuote:
                m_lineState = LineStateEnum::Text;
                break;
            case LineStateEnum::Newline:
                openLine();
                m_lineState = LineStateEnum::Text;
                break;
            case LineStateEnum::Text:
                m_os << string_consts::SV_SPACE;
                break;
            }
        }
        void closeLine()
        {
            assert(m_lineState != LineStateEnum::Newline);
            printDQuote();
            m_os << string_consts::SV_NEWLINE;
            m_lineState = LineStateEnum::Newline;
        }

    private:
        void printToken(const diff::SideEnum side, const std::string_view x)
        {
            if (x == string_consts::SV_NEWLINE) {
                RawAnsi yellow;
                yellow.fg = AnsiColorVariant{AnsiColor16Enum::YELLOW};
                switch (side) {
                case diff::SideEnum::A:
                    yellow.bg = AnsiColorVariant{AnsiColor16Enum::red};
                    break;
                case diff::SideEnum::B:
                    yellow.bg = AnsiColorVariant{AnsiColor16Enum::green};
                    break;
                case diff::SideEnum::Common:
                    break;
                }

                if (m_lineState == LineStateEnum::Newline) {
                    openLine();
                }

                m_os.writeQuotedWithColor({}, yellow, string_consts::SV_NEWLINE, false);

                if (side != diff::SideEnum::A) {
                    closeLine();
                }
                return;
            }

            // REVISIT: cuddle punctuation?
            maybeSpace();

            switch (side) {
            case diff::SideEnum::A: {
                RawAnsi red;
                red.fg = AnsiColorVariant{AnsiColor16Enum::red};
                RawAnsi yellow;
                yellow.fg = AnsiColorVariant{AnsiColor16Enum::YELLOW};
                yellow.bg = AnsiColorVariant{AnsiColor16Enum::red};
                m_os.writeQuotedWithColor(red, yellow, x, false);
                break;
            }
            case diff::SideEnum::B: {
                RawAnsi green;
                green.fg = AnsiColorVariant{AnsiColor16Enum::green};
                RawAnsi yellow;
                yellow.fg = AnsiColorVariant{AnsiColor16Enum::YELLOW};
                yellow.bg = AnsiColorVariant{AnsiColor16Enum::green};
                m_os.writeQuotedWithColor(green, yellow, x, false);
                break;
            }
            case diff::SideEnum::Common: {
                m_os << x;
                break;
            }
            }
        }

    private:
        void virt_report(const diff::SideEnum side, const Range &r) override
        {
            for (const auto &x : r) {
                printToken(side, x);
            }
        }
    };

    {
        MyDiff diff{os};
        diff.compare(diff::Range<std::string_view>::fromVector(aTokens),
                     diff::Range<std::string_view>::fromVector(bTokens));
    }
}

IDiffReporter::~IDiffReporter() = default;
OstreamDiffReporter::~OstreamDiffReporter() = default;

void OstreamDiffReporter::printRemove()
{
    m_os << RawAnsi().withForeground(AnsiColor16Enum::red);
    m_os << "-";
    space();
}

void OstreamDiffReporter::printAdd()
{
    m_os << RawAnsi().withForeground(AnsiColor16Enum::green);
    m_os << "+";
    space();
}

void OstreamDiffReporter::printSep()
{
    m_os << "/";
}

void OstreamDiffReporter::colon()
{
    m_os << ":";
}

void OstreamDiffReporter::space()
{
    m_os << " ";
}

void OstreamDiffReporter::newline()
{
    if ((false)) {
        const AnsiString &string = AnsiString::get_reset_string();
        m_os << string.c_str();
    }
    m_os << "\n";
}

void OstreamDiffReporter::roomPrefix(const RoomHandle &room)
{
    m_os << "Room";
    printSep();
    m_os << room.getIdExternal().asUint32();
}
void OstreamDiffReporter::roomPrefix(const RoomHandle &room, const RoomFieldVariant &var)
{
    roomPrefix(room);
    printSep();
#define X_NOP()
#define X_CASE(UPPER_CASE, CamelCase, Type) \
    case RoomFieldEnum::UPPER_CASE: \
        m_os << #CamelCase; \
        break;
    switch (var.getType()) {
        XFOREACH_ROOM_FIELD(X_CASE, X_NOP)
    case RoomFieldEnum::RESERVED:
        m_os << "ERROR";
    }
#undef X_CASE
#undef X_NOP
}

void OstreamDiffReporter::exitPrefix(const RoomHandle &room, const ExitDirEnum dir)
{
    roomPrefix(room);
    printSep();

    m_os << "Exit";
    printSep();

    auto dirName = to_string_view(dir);
    if (dirName.empty()) {
        m_os << "Error";
        return;
    }

    m_os << static_cast<char>(std::toupper(dirName[0]));
    dirName.remove_prefix(1);
    m_os << dirName;
}
void OstreamDiffReporter::exitPrefix(const RoomHandle &room,
                                     const ExitDirEnum dir,
                                     const ExitFieldVariant &var)
{
    exitPrefix(room, dir);
    printSep();
#define X_NOP()
#define X_CASE(UPPER_CASE, CamelCase) \
    case ExitFieldEnum::UPPER_CASE: { \
        m_os << #CamelCase; \
        break; \
    }
    switch (var.getType()) {
        XFOREACH_EXIT_FIELD(X_CASE, X_NOP)
    }
#undef X_CASE
#undef X_NOP
}

void OstreamDiffReporter::printRoomVariant(void (OstreamDiffReporter::*pfn)(),
                                           const RoomHandle &room,
                                           const RoomFieldVariant &var)
{
    (this->*pfn)();
    roomPrefix(room, var);
    colon();
    space();
    print(var);
    newline();
}

void OstreamDiffReporter::printExitVariant(const RoomHandle &room,
                                           const ExitDirEnum dir,
                                           const ExitFieldVariant &var)
{
    exitPrefix(room, dir, var);
    colon();
    space();
    print(var);
    newline();
}

void OstreamDiffReporter::everything(const RoomHandle &room, void (OstreamDiffReporter::*fn)())
{
    auto plusOrMinus = [this, fn]() { (this->*fn)(); };

    {
        plusOrMinus();
        position(room);
        plusOrMinus();
        status(room);
        plusOrMinus();
    }

    {
#define X_NOP()
#define X_PRINT(UPPER_CASE, CamelCase, Type) \
    do { \
        const auto x = room.get##CamelCase(); \
        if (isDefault(x)) { \
            break; \
        } \
        printRoomVariant(fn, room, RoomFieldVariant{x}); \
    } while (false);

        XFOREACH_ROOM_FIELD(X_PRINT, X_NOP)

#undef X_PRINT
#undef X_NOP
    }

    for (const ExitDirEnum dir : ALL_EXITS7) {
        const auto &ex = room.getExit(dir);
#define X_NOP()
#define X_PRINT(UPPER_CASE, CamelCase) \
    do { \
        const auto &var = ex.get##CamelCase(); \
        if (isDefault(var)) { \
            break; \
        } \
        plusOrMinus(); \
        printExitVariant(room, dir, ExitFieldVariant{var}); \
    } while (false);

        XFOREACH_EXIT_FIELD(X_PRINT, X_NOP)
#undef X_PRINT
#undef X_NOP

        if (ex.getOutgoingSet().empty()) {
            continue;
        }

        plusOrMinus();
        printOutgoing(room, dir);
    }
}

void OstreamDiffReporter::virt_added(const RoomHandle &room)
{
    everything(room, &OstreamDiffReporter::printAdd);
}
void OstreamDiffReporter::virt_removed(const RoomHandle &room)
{
    everything(room, &OstreamDiffReporter::printRemove);
}

void OstreamDiffReporter::serverId(const RoomHandle &room)
{
    constexpr std::string_view name{"ServerId"};
    roomPrefix(room);
    printSep();
    m_os << name;
    m_os << ": ";

    if (auto serverId = room.getServerId(); serverId != INVALID_SERVER_ROOMID) {
        m_os << room.getServerId().asUint32();
    } else {
        m_os << "undefined";
    }
    newline();
}

void OstreamDiffReporter::position(const RoomHandle &room)
{
    constexpr std::string_view name{"Position"};
    roomPrefix(room);
    printSep();
    m_os << name;
    m_os << ": ";

    auto pos = room.getPosition();
    m_os << pos.x;
    m_os << ", ";
    m_os << pos.y;
    m_os << ", ";
    m_os << pos.z;
    newline();
}

void OstreamDiffReporter::status(const RoomHandle &room)
{
    constexpr std::string_view name{"Status"};
    roomPrefix(room);
    printSep();
    m_os << name;
    m_os << ": ";
    m_os << (room.isTemporary() ? "TEMPORARY" : "PERMANENT");
    newline();
}

void OstreamDiffReporter::virt_roomServerIdDifference(const RoomHandle &a, const RoomHandle &b)
{
    assert(a.getServerId() != b.getServerId());

    printRemove();
    serverId(a);

    printAdd();
    serverId(b);
}

void OstreamDiffReporter::virt_roomPositionDifference(const RoomHandle &a, const RoomHandle &b)
{
    assert(a.getPosition() != b.getPosition());

    printRemove();
    position(a);

    printAdd();
    position(b);
}

void OstreamDiffReporter::virt_roomStatusDifference(const RoomHandle &a, const RoomHandle &b)
{
    assert(a.isTemporary() != b.isTemporary());

    printRemove();
    status(a);

    printAdd();
    status(b);
}

void OstreamDiffReporter::printStringVariantDiff(const RoomFieldVariant &aval,
                                                 const RoomFieldVariant &bval)
{
    assert(aval.getType() == bval.getType());

    switch (aval.getType()) {
    case RoomFieldEnum::AREA:
        printDiff(m_os, aval.getArea(), bval.getArea());
        break;
    case RoomFieldEnum::NAME:
        printDiff(m_os, aval.getName(), bval.getName());
        break;
    case RoomFieldEnum::DESC:
        printDiff(m_os, aval.getDescription(), bval.getDescription());
        break;
    case RoomFieldEnum::CONTENTS:
        printDiff(m_os, aval.getContents(), bval.getContents());
        break;
    case RoomFieldEnum::NOTE:
        printDiff(m_os, aval.getNote(), bval.getNote());
        break;

    case RoomFieldEnum::LOAD_FLAGS:
    case RoomFieldEnum::MOB_FLAGS:
    case RoomFieldEnum::ALIGN_TYPE:
    case RoomFieldEnum::LIGHT_TYPE:
    case RoomFieldEnum::PORTABLE_TYPE:
    case RoomFieldEnum::RIDABLE_TYPE:
    case RoomFieldEnum::SUNDEATH_TYPE:
    case RoomFieldEnum::TERRAIN_TYPE:
    case RoomFieldEnum::RESERVED:
        assert(false);
        {
            printRemove();
            print(aval);
            newline();
        }
        {
            printAdd();
            print(bval);
            newline();
        }
        break;
    }
}

void OstreamDiffReporter::printRoomStringVariantDiff(const RoomHandle &a,
                                                     MAYBE_UNUSED const RoomHandle &b,
                                                     const RoomFieldVariant &aval,
                                                     const RoomFieldVariant &bval)
{
    roomPrefix(a, aval);
    colon();
    space();
    newline();

    printStringVariantDiff(aval, bval);
}

void OstreamDiffReporter::virt_roomFieldDifference(const RoomHandle &a,
                                                   const RoomHandle &b,
                                                   const RoomFieldVariant &aval,
                                                   const RoomFieldVariant &bval)
{
    assert(aval.getType() == bval.getType());
    assert(aval != bval);

    switch (aval.getType()) {
    case RoomFieldEnum::AREA:
    case RoomFieldEnum::NAME:
    case RoomFieldEnum::DESC:
    case RoomFieldEnum::CONTENTS:
    case RoomFieldEnum::NOTE:
        printRoomStringVariantDiff(a, b, aval, bval);
        break;

    case RoomFieldEnum::MOB_FLAGS:
    case RoomFieldEnum::LOAD_FLAGS:
    case RoomFieldEnum::PORTABLE_TYPE:
    case RoomFieldEnum::LIGHT_TYPE:
    case RoomFieldEnum::ALIGN_TYPE:
    case RoomFieldEnum::RIDABLE_TYPE:
    case RoomFieldEnum::SUNDEATH_TYPE:
    case RoomFieldEnum::TERRAIN_TYPE:
    case RoomFieldEnum::RESERVED:

        if (!isDefault(aval)) {
            printRoomVariant(&OstreamDiffReporter::printRemove, a, aval);
        }

        if (!isDefault(bval)) {
            printRoomVariant(&OstreamDiffReporter::printAdd, b, bval);
        }

        break;
    }
}

void OstreamDiffReporter::virt_exitFieldDifference(const RoomHandle &a,
                                                   const RoomHandle &b,
                                                   const ExitDirEnum dir,
                                                   const ExitFieldVariant &aval,
                                                   const ExitFieldVariant &bval)
{
    assert(aval.getType() == bval.getType());
    assert(aval != bval);

    if (!isDefault(aval)) {
        printRemove();
        printExitVariant(a, dir, aval);
    }

    if (!isDefault(bval)) {
        printAdd();
        printExitVariant(b, dir, bval);
    }
}

void OstreamDiffReporter::printOutgoing(const RoomHandle &room, const ExitDirEnum dir)
{
    const auto &map = room.getMap();
    const World &w = map.getWorld();

    exitPrefix(room, dir);
    printSep();
    m_os << "Outgoing";

    colon();
    space();

    // m_os << "{";
    {
        const auto &ex = room.getExit(dir);
        if (!ex.getOutgoingSet().empty()) {
            ExternalRoomIdSet ext;
            for (const RoomId to : ex.getOutgoingSet()) {
                ext.insert(w.convertToExternal(to));
            }

            bool first = true;
            for (const ExternalRoomId extId : ext) {
                if (first) {
                    first = false;
                } else {
                    space();
                }
                m_os << extId.value();
            }
        }
    }
    // m_os << "}";
    newline();
}

void OstreamDiffReporter::virt_exitOutgoingDifference(const RoomHandle &a,
                                                      const RoomHandle &b,
                                                      const ExitDirEnum dir,
                                                      const TinyRoomIdSet &aset,
                                                      const TinyRoomIdSet &bset)
{
    assert(aset != bset);

    if (!aset.empty()) {
        printRemove();
        printOutgoing(a, dir);
    }

    if (!bset.empty()) {
        printAdd();
        printOutgoing(b, dir);
    }
}

void OstreamDiffReporter::print(const RoomFieldVariant &var)
{
    var.acceptVisitor([this](const auto &x) { print(x); });
}

void OstreamDiffReporter::print(const ExitFieldVariant &var)
{
    var.acceptVisitor([this](const auto &x) { print(x); });
}

void OstreamDiffReporter::printQuoted(const std::string_view sv)
{
    const auto yellowAnsi = getRawAnsi(AnsiColor16Enum::yellow);
    m_os.writeQuotedWithColor(m_os.getNextAnsi(), yellowAnsi, sv);
}
