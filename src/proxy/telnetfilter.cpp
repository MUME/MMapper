// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "telnetfilter.h"

#include "../global/ConfigConsts.h"
#include "../global/Consts.h"
#include "../global/PrintUtils.h"
#include "../global/tests.h"

#include <iostream>

#include <QByteArray>

namespace { // anonymous
NODISCARD bool isBackspace(const RawBytes &bytes)
{
    return !bytes.isEmpty() && bytes.back() == char_consts::C_BACKSPACE;
}

NODISCARD bool isNewline(const RawBytes &bytes)
{
    return !bytes.isEmpty() && bytes.back() == char_consts::C_NEWLINE;
}

NODISCARD bool isCrlf(const RawBytes &bytes)
{
    return isNewline(bytes) && bytes.size() >= 2
           && bytes.at(bytes.size() - 2) == char_consts::C_CARRIAGE_RETURN;
}

} // namespace

bool TelnetLineFilter::endsWith(const char c) const
{
    return !m_buffer.isEmpty() && m_buffer.back() == c;
}

void TelnetLineFilter::fwd(const TelnetDataEnum type)
{
    if (IS_DEBUG_BUILD) {
        switch (type) {
        case TelnetDataEnum::Empty:
            assert(m_buffer.isEmpty());
            break;
        case TelnetDataEnum::Prompt:
            assert(!isNewline(m_buffer));
            break;
        case TelnetDataEnum::CRLF:
            assert(isCrlf(m_buffer));
            break;
        case TelnetDataEnum::LF:
            assert(isNewline(m_buffer) && !isCrlf(m_buffer));
            break;
        case TelnetDataEnum::Backspace:
            assert(m_reportBackspaces == OptionBackspacesEnum::Yes);
            assert(isBackspace(m_buffer));
            break;
        }
    }

    if (m_callback != nullptr) {
        try {
            m_callback(TelnetData{m_buffer, type});
        } catch (...) {
            m_buffer.clear();
            throw;
        }
    }
    m_buffer.clear();
}

void TelnetLineFilter::receive(const RawBytes &input, const bool goAhead)
{
    using namespace char_consts;

    for (const char c : input) {
        switch (c) {
        case C_BACKSPACE:
            m_buffer += c;
            if (m_reportBackspaces == OptionBackspacesEnum::Yes) {
                fwd(TelnetDataEnum::Backspace);
            }
            break;

        case C_CARRIAGE_RETURN:
            m_buffer += c;
            break;

        case C_NEWLINE: {
            const bool wasCr = endsWith(C_CARRIAGE_RETURN);
            m_buffer += c;
            fwd(wasCr ? TelnetDataEnum::CRLF : TelnetDataEnum::LF);
            break;
        }

        default:
            m_buffer += c;
            break;
        }
    }

    if (!m_buffer.isEmpty() && goAhead) {
        // This should work fine with utf8 as long as a multibyte codepoints aren't split
        // by an IAC GA.
        fwd(TelnetDataEnum::Prompt);
    }
}

namespace test {
void test_telnetfilter()
{
    struct NODISCARD TestHelper final
    {
        using Results = std::vector<TelnetData>;

    private:
        Results m_results;
        TelnetLineFilter m_filter;

    public:
        explicit TestHelper(TelnetLineFilter::OptionBackspacesEnum reportBackspaces)
            : m_filter{reportBackspaces,
                       [this](const TelnetData &data) { m_results.push_back(data); }}
        {}

    public:
        NODISCARD static auto from_mud()
        {
            return TestHelper{TelnetLineFilter::OptionBackspacesEnum::Yes};
        }
        NODISCARD static auto from_user()
        {
            return TestHelper{TelnetLineFilter::OptionBackspacesEnum::No};
        }

    public:
        void receive(const RawBytes &input, const bool goAhead)
        {
            m_filter.receive(input, goAhead);
        }

    public:
        void verify_size(const size_t expect) const { TEST_ASSERT(m_results.size() == expect); }
        void verify_result(const size_t n,
                           const QByteArray &expect_line,
                           const TelnetDataEnum expect_type) const
        {
            TEST_ASSERT(m_results.size() > n);
            const auto &data = m_results[n];
            TEST_ASSERT(data.line.getQByteArray() == expect_line);
            TEST_ASSERT(data.type == expect_type);
        }

    public:
        void print_results(std::ostream &os) const
        {
            size_t n = 0;
            for (const TelnetData &data : m_results) {
                os << "[" << n++ << "] = ";

                print_string_quoted(os, data.line.getQByteArray().toStdString());
#define X_CASE(x) \
    case TelnetDataEnum::x: \
        os << " with " << #x; \
        break
                switch (data.type) {
                    X_CASE(Empty);
                    X_CASE(Prompt);
                    X_CASE(CRLF);
                    X_CASE(LF);
                    X_CASE(Backspace);
                }
#undef X_CASE
                os << "\n";
            }
        }
    };

    {
        // state is carried over multiple receives
        auto fromUser = TestHelper::from_user();
        fromUser.receive(RawBytes{"a"}, false);
        fromUser.receive(RawBytes{"b"}, true);
        fromUser.receive(RawBytes{"c"}, false);
        fromUser.receive(RawBytes{"\n"}, false);
        fromUser.verify_size(2);
        fromUser.verify_result(0, "ab", TelnetDataEnum::Prompt);
        fromUser.verify_result(1, "c\n", TelnetDataEnum::LF);
    }

    {
        // LFCR is not recognized; nor is LFCRLF.
        auto fromMud = TestHelper::from_mud();
        fromMud.receive(RawBytes{"\n\r\n\r"}, true);
        // note: This test case fails with the previous version of the filter;
        // it yields [0] = "\n\r\n" with CRLF, [1] = "\r" with Prompt.
        if ((false)) {
            fromMud.print_results(std::cout);
        }
        fromMud.verify_size(3);
        fromMud.verify_result(0, "\n", TelnetDataEnum::LF);
        fromMud.verify_result(1, "\r\n", TelnetDataEnum::CRLF);
        fromMud.verify_result(2, "\r", TelnetDataEnum::Prompt);
    }

    const QByteArray with_lf_only = "\n\n\b0\b1\n2\b\n3\b4\n5";
    const auto with_crlf = [&with_lf_only]() {
        auto copy = with_lf_only;
        copy.replace("\n", "\r\n");
        return copy;
    }();

    {
        auto fromMud = TestHelper::from_mud();
        fromMud.receive(RawBytes{with_lf_only}, true);
        // note: This test case fails with the previous version of the filter;
        // it yields [0] = "\n\n\b" with Backspace.
        if ((false)) {
            fromMud.print_results(std::cout);
        }
        fromMud.verify_size(10);
        fromMud.verify_result(0, "\n", TelnetDataEnum::LF);
        fromMud.verify_result(1, "\n", TelnetDataEnum::LF);
        fromMud.verify_result(2, "\b", TelnetDataEnum::Backspace);
        fromMud.verify_result(3, "0\b", TelnetDataEnum::Backspace);
        fromMud.verify_result(4, "1\n", TelnetDataEnum::LF);
        fromMud.verify_result(5, "2\b", TelnetDataEnum::Backspace);
        fromMud.verify_result(6, "\n", TelnetDataEnum::LF);
        fromMud.verify_result(7, "3\b", TelnetDataEnum::Backspace);
        fromMud.verify_result(8, "4\n", TelnetDataEnum::LF);
        fromMud.verify_result(9, "5", TelnetDataEnum::Prompt);
    }

    {
        auto fromMud = TestHelper::from_mud();
        fromMud.receive(RawBytes{with_crlf}, true);
        fromMud.verify_size(10);
        fromMud.verify_result(0, "\r\n", TelnetDataEnum::CRLF);
        fromMud.verify_result(1, "\r\n", TelnetDataEnum::CRLF);
        fromMud.verify_result(2, "\b", TelnetDataEnum::Backspace);
        fromMud.verify_result(3, "0\b", TelnetDataEnum::Backspace);
        fromMud.verify_result(4, "1\r\n", TelnetDataEnum::CRLF);
        fromMud.verify_result(5, "2\b", TelnetDataEnum::Backspace);
        fromMud.verify_result(6, "\r\n", TelnetDataEnum::CRLF);
        fromMud.verify_result(7, "3\b", TelnetDataEnum::Backspace);
        fromMud.verify_result(8, "4\r\n", TelnetDataEnum::CRLF);
        fromMud.verify_result(9, "5", TelnetDataEnum::Prompt);
    }

    {
        auto fromUser = TestHelper::from_user();
        fromUser.receive(RawBytes{with_lf_only}, true);
        fromUser.verify_size(6);
        fromUser.verify_result(0, "\n", TelnetDataEnum::LF);
        fromUser.verify_result(1, "\n", TelnetDataEnum::LF);
        fromUser.verify_result(2, "\b0\b1\n", TelnetDataEnum::LF);
        fromUser.verify_result(3, "2\b\n", TelnetDataEnum::LF);
        fromUser.verify_result(4, "3\b4\n", TelnetDataEnum::LF);
        fromUser.verify_result(5, "5", TelnetDataEnum::Prompt);
    }

    {
        auto fromUser = TestHelper::from_user();
        fromUser.receive(RawBytes{with_crlf}, true);
        fromUser.verify_size(6);
        fromUser.verify_result(0, "\r\n", TelnetDataEnum::CRLF);
        fromUser.verify_result(1, "\r\n", TelnetDataEnum::CRLF);
        fromUser.verify_result(2, "\b0\b1\r\n", TelnetDataEnum::CRLF);
        fromUser.verify_result(3, "2\b\r\n", TelnetDataEnum::CRLF);
        fromUser.verify_result(4, "3\b4\r\n", TelnetDataEnum::CRLF);
        fromUser.verify_result(5, "5", TelnetDataEnum::Prompt);
    }
}

} // namespace test
