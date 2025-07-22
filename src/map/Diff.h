#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "../global/AnsiOstream.h"
#include "../global/PrintUtils.h"
#include "../global/RuleOf5.h"
#include "../global/macros.h"
#include "ChangePrinter.h"
#include "Changes.h"

#include <ostream>

struct NODISCARD IDiffReporter
{
protected:
    IDiffReporter() = default;

public:
    virtual ~IDiffReporter();
    DELETE_CTORS_AND_ASSIGN_OPS(IDiffReporter);

private:
    virtual void virt_added(const RoomHandle &) = 0;
    virtual void virt_removed(const RoomHandle &) = 0;

private:
    virtual void virt_roomServerIdDifference(const RoomHandle &, const RoomHandle &) = 0;
    virtual void virt_roomPositionDifference(const RoomHandle &, const RoomHandle &) = 0;
    virtual void virt_roomStatusDifference(const RoomHandle &, const RoomHandle &) = 0;
    virtual void virt_roomFieldDifference(const RoomHandle &,
                                          const RoomHandle &,
                                          const RoomFieldVariant &from,
                                          const RoomFieldVariant &to)
        = 0;

    virtual void virt_exitFieldDifference(const RoomHandle &,
                                          const RoomHandle &,
                                          ExitDirEnum,
                                          const ExitFieldVariant &,
                                          const ExitFieldVariant &)
        = 0;
    virtual void virt_exitOutgoingDifference(const RoomHandle &,
                                             const RoomHandle &,
                                             ExitDirEnum,
                                             const TinyRoomIdSet &,
                                             const TinyRoomIdSet &)
        = 0;

public:
    void added(const RoomHandle &room) { virt_added(room); }
    void removed(const RoomHandle &room) { virt_removed(room); }

public:
    void roomServerIdDifference(const RoomHandle &a, const RoomHandle &b)
    {
        virt_roomServerIdDifference(a, b);
    }
    void roomPositionDifference(const RoomHandle &a, const RoomHandle &b)
    {
        virt_roomPositionDifference(a, b);
    }
    void roomStatusDifference(const RoomHandle &a, const RoomHandle &b)
    {
        virt_roomStatusDifference(a, b);
    }
    void roomFieldDifference(const RoomHandle &a,
                             const RoomHandle &b,
                             const RoomFieldVariant &from,
                             const RoomFieldVariant &to)
    {
        virt_roomFieldDifference(a, b, from, to);
    }

    void exitFieldDifference(const RoomHandle &a,
                             const RoomHandle &b,
                             const ExitDirEnum dir,
                             const ExitFieldVariant &avar,
                             const ExitFieldVariant &bvar)
    {
        virt_exitFieldDifference(a, b, dir, avar, bvar);
    }
    void exitOutgoingDifference(const RoomHandle &a,
                                const RoomHandle &b,
                                const ExitDirEnum dir,
                                const TinyRoomIdSet &aset,
                                const TinyRoomIdSet &bset)
    {
        virt_exitOutgoingDifference(a, b, dir, aset, bset);
    }
};

/* lvalue version */
void compare(IDiffReporter &, const RoomHandle &a, const RoomHandle &b);
/* rvalue version calls lvalue version */
inline void compare(IDiffReporter &&reporter, const RoomHandle &a, const RoomHandle &b)
{
    compare(reporter, a, b);
}

struct NODISCARD OstreamDiffReporter : public IDiffReporter
{
private:
    AnsiOstream &m_os;

public:
    explicit OstreamDiffReporter(AnsiOstream &os)
        : m_os{os}
    {}
    ~OstreamDiffReporter() override;
    DELETE_CTORS_AND_ASSIGN_OPS(OstreamDiffReporter);

private:
    void virt_added(const RoomHandle &) final;
    void virt_removed(const RoomHandle &) final;

private:
    void virt_roomServerIdDifference(const RoomHandle &, const RoomHandle &) final;
    void virt_roomPositionDifference(const RoomHandle &, const RoomHandle &) final;
    void virt_roomStatusDifference(const RoomHandle &, const RoomHandle &) final;

    void printStringVariantDiff(const RoomFieldVariant &aval, const RoomFieldVariant &bval);

    void printRoomStringVariantDiff(const RoomHandle &a,
                                    const RoomHandle &b,
                                    const RoomFieldVariant &aval,
                                    const RoomFieldVariant &bval);

    void virt_roomFieldDifference(const RoomHandle &a,
                                  const RoomHandle &b,
                                  const RoomFieldVariant &,
                                  const RoomFieldVariant &) final;
    void virt_exitFieldDifference(const RoomHandle &a,
                                  const RoomHandle &b,
                                  ExitDirEnum dir,
                                  const ExitFieldVariant &,
                                  const ExitFieldVariant &) final;

    void virt_exitOutgoingDifference(const RoomHandle &,
                                     const RoomHandle &,
                                     ExitDirEnum,
                                     const TinyRoomIdSet &,
                                     const TinyRoomIdSet &) final;

private:
    void newline();
    void colon();
    void space();
    void printRemove();
    void printAdd();
    void printSep();
    void roomPrefix(const RoomHandle &);
    void roomPrefix(const RoomHandle &, const RoomFieldVariant &);
    void exitPrefix(const RoomHandle &, ExitDirEnum);
    void exitPrefix(const RoomHandle &, ExitDirEnum, const ExitFieldVariant &);
    void printRoomVariant(void (OstreamDiffReporter::*pfn)(),
                          const RoomHandle &,
                          const RoomFieldVariant &);
    void printExitVariant(const RoomHandle &, ExitDirEnum, const ExitFieldVariant &);
    void printOutgoing(const RoomHandle &room, ExitDirEnum dir);

private:
    void serverId(const RoomHandle &);
    void position(const RoomHandle &);
    void status(const RoomHandle &);
    void everything(const RoomHandle &, void (OstreamDiffReporter::*fn)());

private:
    void print(const RoomFieldVariant &);
    void print(const ExitFieldVariant &);

private:
    void printQuoted(std::string_view sv);

    template<typename T>
    auto printQuoted(const TaggedStringUtf8<T> &s)
    {
        printQuoted(s.getStdStringUtf8());
    }
    template<typename T>
    auto printQuoted(const TaggedBoxedStringUtf8<T> &s)
    {
        printQuoted(s.getStdStringViewUtf8());
    }

    template<typename E>
    auto printEnum(const E x) -> std::enable_if_t<std::is_enum_v<E>>
    {
        m_os << to_string_view(x);
    }

    template<typename Flags>
    void printFlags(const Flags flags)
    {
        // assumes it's templated based on Flags in Flags.h
        using Flag = typename decltype(flags)::Flag;
        static_assert(std::is_enum_v<Flag>);

        bool first = true;
        // m_os << "[";
        for (const Flag flag : flags) {
            if (first) {
                first = false;
            } else {
                space();
            }
            printEnum(flag);
        }
        // m_os << "]";
    }

private:
    void print(const RoomArea &area) { printQuoted(area); }
    void print(const RoomName &name) { printQuoted(name); }
    void print(const RoomDesc &desc) { printQuoted(desc); }
    void print(const RoomContents &desc) { printQuoted(desc); }
    void print(const RoomNote &note) { printQuoted(note); }
    void print(const DoorName &name) { printQuoted(name); }

    void print(const RoomMobFlags flags) { printFlags(flags); }
    void print(const RoomLoadFlags flags) { printFlags(flags); }
    void print(const ExitFlags flags) { printFlags(flags); }
    void print(const DoorFlags flags) { printFlags(flags); }

    void print(const RoomPortableEnum x) { printEnum(x); }
    void print(const RoomLightEnum x) { printEnum(x); }
    void print(const RoomAlignEnum x) { printEnum(x); }
    void print(const RoomRidableEnum x) { printEnum(x); }
    void print(const RoomSundeathEnum x) { printEnum(x); }
    void print(const RoomTerrainEnum x) { printEnum(x); }
};

extern void printDiff(AnsiOstream &os, std::string_view a, std::string_view b);

namespace test {
extern void testMapDiff();
} // namespace test
