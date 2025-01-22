#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/macros.h"
#include "TaggedBytes.h"

#include <functional>

enum class NODISCARD TelnetDataEnum : uint8_t { Empty, Prompt, CRLF, LF, Backspace };

struct NODISCARD TelnetData final
{
    RawBytes line;
    TelnetDataEnum type = TelnetDataEnum::Empty;
};

class NODISCARD TelnetLineFilter final
{
public:
    using Callback = std::function<void(const TelnetData &)>;
    enum class NODISCARD OptionBackspacesEnum : uint8_t { No, Yes };

private:
    RawBytes m_buffer;
    Callback m_callback;
    OptionBackspacesEnum m_reportBackspaces = OptionBackspacesEnum::No;

public:
    explicit TelnetLineFilter(const OptionBackspacesEnum reportBackspaces, Callback callback)
        : m_callback{std::move(callback)}
        , m_reportBackspaces{reportBackspaces}
    {
        assert(m_callback != nullptr);
    }

private:
    NODISCARD bool endsWith(char c) const;
    void fwd(TelnetDataEnum);

public:
    void receive(const RawBytes &input, bool goAhead);
};

namespace test {
extern void test_telnetfilter();
} // namespace test
