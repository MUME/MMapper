// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2021 The MMapper Authors

#include "Diff.h"

#include "ConfigConsts.h"
#include "logging.h"
#include "tests.h"

#include <ostream>
#include <sstream>
#include <vector>

namespace test {
void testDiff()
{
    using namespace diff;
    struct NODISCARD Scorer final
    {
        NODISCARD float operator()(const char ta, const char tb) const
        {
            if (ta != tb) {
                return 0.f;
            }

            return ascii::isSpace(ta) ? 0.01f : 1.f;
        }
    };

    struct NODISCARD MyDiff final : public diff::Diff<char, Scorer>
    {
    private:
        std::ostream &m_os;

    public:
        bool brackets = true;

    public:
        explicit MyDiff(std::ostream &os)
            : m_os{os}
        {}

    private:
        void virt_report(SideEnum side, const Range &r) final
        {
            switch (side) {
            case SideEnum::Common:
                m_os << "\033[0m";
                for (char c : r) {
                    m_os << c;
                }
                m_os << "\033[0m";
                break;
            case SideEnum::A:
                if (brackets) {
                    m_os << "\033[0;31m[-";
                }
                m_os << "\033[0;101m";
                for (char c : r) {
                    m_os << c;
                }
                m_os << "\033[0;31m";
                if (brackets) {
                    m_os << "-]\033[0m";
                }
                break;
            case SideEnum::B:
                if (brackets) {
                    m_os << "\033[0;32m{+";
                }
                m_os << "\033[0;102m";
                for (char c : r) {
                    m_os << c;
                }
                m_os << "\033[0;32m";
                if (brackets) {
                    m_os << "+}\033[0m";
                }
                break;
            }
        }
        // NODISCARD float virt_scoreToken(const Token &ta, const Token &tb) const
    };

    const auto r1 = Range<char>::fromStringView("The quick brown fox jumps over the lazy dog.");
    const auto r2 = Range<char>::fromStringView("The lazy fox hides from the dog.");

    const auto s1 = [&r1, &r2]() -> std::string {
        std::ostringstream os;
        MyDiff lcs{os};
        lcs.compare(r1, r2);
        return std::move(os).str();
    }();
    const auto s2 = [&r1, &r2]() -> std::string {
        std::ostringstream os;
        MyDiff lcs{os};
        lcs.compare(r2, r1);
        return std::move(os).str();
    }();

    const std::string_view RESET = "\x1B[0m";
    const std::string_view FG_RED = "\x1B[0;31m";
    const std::string_view FG_GRN = "\x1B[0;32m";
    const std::string_view BG_RED = "\x1B[0;101m";
    const std::string_view BG_GRN = "\x1B[0;102m";
    const std::string_view BEG_REM = "[-";
    const std::string_view END_REM = "-]";
    const std::string_view BEG_ADD = "{+";
    const std::string_view END_ADD = "+}";

    const std::vector<std::string_view> expect_s1_svs
        = {RESET,   "The ", RESET,   FG_RED,  BEG_REM, BG_RED,   "quick brown", FG_RED,  END_REM,
           RESET,   FG_GRN, BEG_ADD, BG_GRN,  "lazy",  FG_GRN,   END_ADD,       RESET,   RESET,
           " fox",  RESET,  FG_RED,  BEG_REM, BG_RED,  " jumps", FG_RED,        END_REM, RESET,
           RESET,   " ",    RESET,   FG_RED,  BEG_REM, BG_RED,   "ov",          FG_RED,  END_REM,
           RESET,   FG_GRN, BEG_ADD, BG_GRN,  "hid",   FG_GRN,   END_ADD,       RESET,   RESET,
           "e",     RESET,  FG_GRN,  BEG_ADD, BG_GRN,  "s f",    FG_GRN,        END_ADD, RESET,
           RESET,   "r",    RESET,   FG_GRN,  BEG_ADD, BG_GRN,   "om",          FG_GRN,  END_ADD,
           RESET,   RESET,  " the",  RESET,   FG_RED,  BEG_REM,  BG_RED,        " lazy", FG_RED,
           END_REM, RESET,  RESET,   " dog.", RESET};
    const std::vector<std::string_view> expect_s2_svs
        = {RESET,   "The ", RESET,   FG_RED,  BEG_REM,       BG_RED,   "lazy",  FG_RED,  END_REM,
           RESET,   FG_GRN, BEG_ADD, BG_GRN,  "quick brown", FG_GRN,   END_ADD, RESET,   RESET,
           " fox",  RESET,  FG_GRN,  BEG_ADD, BG_GRN,        " jumps", FG_GRN,  END_ADD, RESET,
           RESET,   " ",    RESET,   FG_RED,  BEG_REM,       BG_RED,   "hid",   FG_RED,  END_REM,
           RESET,   FG_GRN, BEG_ADD, BG_GRN,  "ov",          FG_GRN,   END_ADD, RESET,   RESET,
           "e",     RESET,  FG_RED,  BEG_REM, BG_RED,        "s f",    FG_RED,  END_REM, RESET,
           RESET,   "r",    RESET,   FG_RED,  BEG_REM,       BG_RED,   "om",    FG_RED,  END_REM,
           RESET,   RESET,  " the",  RESET,   FG_GRN,        BEG_ADD,  BG_GRN,  " lazy", FG_GRN,
           END_ADD, RESET,  RESET,   " dog.", RESET};

    auto concat = [](const auto &svs) -> std::string {
        std::ostringstream os;
        for (auto &sv : svs) {
            os << sv;
        }
        return std::move(os).str();
    };

    const auto expect_s1 = concat(expect_s1_svs);
    const auto expect_s2 = concat(expect_s2_svs);

    MMLOG() << "s1: " << s1;
    MMLOG() << "s2: " << s2;

    TEST_ASSERT(s1 == expect_s1);
    TEST_ASSERT(s2 == expect_s2);
}

} // namespace test
