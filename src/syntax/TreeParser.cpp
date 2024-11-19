// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "TreeParser.h"

#include "../global/AnsiTextUtils.h"
#include "../global/Consts.h"
#include "../global/PrintUtils.h"
#include "../global/unquote.h"
#include "SyntaxArgs.h"
#include "TokenMatcher.h"

#include <memory>
#include <optional>
#include <ostream>
#include <sstream>
#include <vector>

namespace syntax {

TreeParser::TreeParser(SharedConstSublist syntaxRoot, User &user)
    : m_syntaxRoot(std::move(syntaxRoot))
    , m_user{user}
{}

bool TreeParser::parse(const ParserInput &input)
{
    if (syntaxOnly(input))
        return true;

    bool isFull = false;
    const auto isHelpRequest = [&isFull](const std::string_view s) -> bool {
        if (s == "??") {
            isFull = true;
            return true;
        }

        return s == "?" || s == "/?" || s == "/h" || s == "/help" || s == "-h" || s == "--help";
    };

    auto copy = input;
    if (isHelpRequest(copy.back())) {
        copy = input.rmid(1);
    }

    m_user.getOstream() << (isFull ? "Full" : "Basic") << " syntax help for [" << copy << "]"
                        << std::endl;

    help(copy, isFull);
    return false;
}

bool TreeParser::syntaxOnly(const ParserInput &input)
{
    const Sublist &node = deref(m_syntaxRoot);
    return syntaxRecurseFirst(node, input, nullptr).isSuccess();
}

ParseResult TreeParser::syntaxRecurseFirst(const Sublist &node,
                                           const ParserInput &input,
                                           const Pair *const matchedArgs)
{
    if (node.holdsNestedSublist()) {
        return recurseNewSublist(node, input, matchedArgs);
    } else {
        return recurseTokenMatcher(node, input, matchedArgs);
    }
}

ParseResult TreeParser::recurseNewSublist(const Sublist &node,
                                          const ParserInput &input,
                                          const Pair *const matchedArgs)
{
    ParseResult tmp = syntaxRecurseFirst(deref(node.getNestedSublist()), input, matchedArgs);
    if (tmp.isSuccess()) {
        return tmp;
    }

    // Note: No support for stacking arguments spilled from a list.
    return syntaxRecurseNext(node, input, matchedArgs);
}

ParseResult TreeParser::recurseTokenMatcher(const Sublist &node,
                                            const ParserInput &input,
                                            const Pair *const matchedArgs)
{
    MatchResult tmp = node.getTokenMatcher().tryMatch(input, nullptr);
    if (!tmp) {
        return ParseResult::failure(input);
    }

    if (!tmp.optValue) {
        return syntaxRecurseNext(node, tmp.unmatched, matchedArgs);
    }

    const Pair p{tmp.optValue.value(), matchedArgs};
    return syntaxRecurseNext(node, tmp.unmatched, &p);
}

ParseResult TreeParser::syntaxRecurseNext(const Sublist &node,
                                          const ParserInput &input,
                                          const Pair *const matchedArgs)
{
    if (!node.hasNextNode())
        return recurseAccept(node, input, matchedArgs);

    if (const SharedConstSublist &tmp = node.getNext())
        return syntaxRecurseFirst(deref(tmp), input, matchedArgs);

    return ParseResult::failure(input);
}

ParseResult TreeParser::recurseAccept(const Sublist &node,
                                      const ParserInput &input,
                                      const Pair *matchedArgs)
{
    if (!input.empty())
        return ParseResult::failure(input);

    node.getAcceptFn().call(m_user, matchedArgs);
    return ParseResult::success(input);
}

struct NODISCARD HelpFrame final : IMatchErrorLogger
{
public:
    std::ostream &os; // prevents move ctor and operator=

private:
    size_t m_indent = 0;
    std::vector<std::string> m_helps;
    std::optional<std::string> m_accept;
    std::vector<std::string> m_errors;
    bool m_failed = false;

public:
    explicit HelpFrame(std::ostream &os_)
        : os{os_}
    {}
    DEFAULT_COPY_CTOR(HelpFrame);
    // TODO: change this to DELETE_MOVE_CTOR (compiler bug: g++ 7.4 uses move ctor in NRVO).
    DEFAULT_MOVE_CTOR(HelpFrame);
    DELETE_ASSIGN_OPS(HelpFrame);
    ~HelpFrame() final;

public:
    NODISCARD bool getFailed() const { return m_failed; }
    void setFailed() { m_failed = true; }
    NODISCARD bool empty() { return m_helps.empty() && !m_accept && m_errors.empty(); }
    void addAccept(std::string acc)
    {
        // REVISIT: this should probably be colored,
        // but we don't have the info for that here.
        if (m_helps.empty())
            m_helps.emplace_back("<(empty)>");
        m_accept = std::move(acc);
    }
    void addHelp(std::string help) { m_helps.emplace_back(help); }
    void addHelp(const TokenMatcher &tokenMatcher, std::optional<MatchTypeEnum> type);
    void flush();
    NODISCARD HelpFrame makeChild();

private:
    void virt_logError(std::string s) override { m_errors.emplace_back(s); }
};

HelpFrame::~HelpFrame()
{
    flush();
}

void HelpFrame::flush()
{
    using namespace char_consts;

    if (empty())
        return;

    if (!m_helps.empty() || m_accept) {
        std::ostringstream ss;

        ss << std::string(2 * m_indent, C_SPACE);

        if (!m_helps.empty()) {
            bool first = true;
            for (const auto &h : m_helps) {
                if (first)
                    first = false;
                else
                    ss << C_SPACE;
                ss << h;
            }
        }

        auto str = ss.str();
        os << str;

        // TODO: convert QString ansi stuff to std::string_view instead of QStringView
        static const auto getLengthAnsiAware = [](const std::string_view sv) -> size_t {
            size_t len = 0;
            bool inEsc = false;
            for (char c : sv) {
                if (c == C_ESC)
                    inEsc = true;
                else if (inEsc) {
                    if (c == 'm')
                        inEsc = false;
                    else
                        assert(std::isdigit(c) || c == C_OPEN_BRACKET || c == C_SEMICOLON
                               || c == C_COLON);
                } else {
                    ++len;
                }
            }
            return len;
        };

        size_t pos = getLengthAnsiAware(str);

        if (m_accept) {
            if (!m_helps.empty()) {
                pos += 1;
                os << C_SPACE;
            }

            const auto &acc = m_accept.value();
            size_t accLen = acc.length();
            const auto wouldEndAt = pos + accLen;
            const size_t rightMargin = 80;
            if (wouldEndAt <= rightMargin) {
                os << std::string(rightMargin - wouldEndAt, C_SPACE);
            } else if (accLen + 2 <= rightMargin) {
                os << std::endl;
                os << std::string(rightMargin - accLen - 2, C_SPACE);
                os << "# ";
            } else {
                os << std::endl;
                os << "# ";
            }
            os << acc;
        }
        os << std::endl;
    }

    if (!m_errors.empty()) {
        for (const std::string &w : m_errors) {
            os << std::string(2 * m_indent, C_SPACE);
            os << " ^ warning: " << w << std::endl;
        }

        os << std::endl; // blank line
    }

    m_helps.clear();
    m_accept.reset();
    m_errors.clear();
    ++m_indent;
}

HelpFrame HelpFrame::makeChild()
{
    flush();
    HelpFrame child = *this; // copy ctor
    return child; // c++17 spec says NRVO elides the move constructor here, but g++ 7.4 uses move ctor.
}

class NODISCARD TreeParser::HelpCommon final
{
protected:
    const bool m_isFull = true;

public:
    explicit HelpCommon(const bool isFull)
        : m_isFull(isFull)
    {}

public:
    NODISCARD ParseResult syntaxRecurseFirst(const Sublist &node,
                                             const ParserInput &input,
                                             HelpFrame &frame);
    NODISCARD ParseResult recurseNewSublist(const Sublist &node,
                                            const ParserInput &input,
                                            HelpFrame &frame);
    NODISCARD ParseResult recurseTokenMatcher(const Sublist &node,
                                              const ParserInput &input,
                                              HelpFrame &frame);
    NODISCARD ParseResult syntaxRecurseNext(const Sublist &node,
                                            const ParserInput &input,
                                            HelpFrame &frame);
    NODISCARD ParseResult recurseAccept(const Sublist &node,
                                        const ParserInput &input,
                                        HelpFrame &frame);
};

void HelpFrame::addHelp(const TokenMatcher &tokenMatcher, std::optional<MatchTypeEnum> type)
{
    if (!type) {
        std::ostringstream ss;
        ss << tokenMatcher;
        addHelp(ss.str());
        return;
    }

    RawAnsi raw;
    raw.setBold();

    switch (type.value()) {
    case MatchTypeEnum::Fail:
        raw.fg = AnsiColorVariant{AnsiColor16Enum::red};
        break;
    case MatchTypeEnum::Partial:
        raw.fg = AnsiColorVariant{AnsiColor16Enum::yellow};
        break;
    case MatchTypeEnum::Pass:
        raw.fg = AnsiColorVariant{AnsiColor16Enum::cyan};
        break;
    }

    // REVISIT: store this as Ansi instead of the string version.
    std::ostringstream ss;
    to_stream_as_reset(ss, ANSI_COLOR_SUPPORT_HI, raw);
    ss << tokenMatcher;
    ss << reset_ansi;
    addHelp(std::move(ss).str());
}

ParseResult TreeParser::HelpCommon::syntaxRecurseFirst(const Sublist &node,
                                                       const ParserInput &input,
                                                       HelpFrame &frame)
{
    if (node.holdsNestedSublist()) {
        return recurseNewSublist(node, input, frame);
    } else {
        return recurseTokenMatcher(node, input, frame);
    }
}

ParseResult TreeParser::HelpCommon::recurseNewSublist(const Sublist &node,
                                                      const ParserInput &input,
                                                      HelpFrame &frame)
{
    {
        HelpFrame newFrame = frame.makeChild();
        ParseResult tmp = syntaxRecurseFirst(deref(node.getNestedSublist()), input, newFrame);
        if (tmp.isSuccess()) {
            return tmp;
        }
    }

    // Note: No support for stacking arguments spilled from a list.
    return syntaxRecurseNext(node, input, frame);
}

ParseResult TreeParser::HelpCommon::recurseTokenMatcher(const Sublist &node,
                                                        const ParserInput &input,
                                                        HelpFrame &frame)
{
    const TokenMatcher &tokenMatcher = node.getTokenMatcher();

    if (frame.getFailed()) {
        frame.addHelp(tokenMatcher, std::nullopt);
        return syntaxRecurseNext(node, input, frame);
    }

    MatchResult tmp = tokenMatcher.tryMatch(input, &frame);
    if (tmp) {
        frame.addHelp(tokenMatcher, MatchTypeEnum::Pass);
        return syntaxRecurseNext(node, tmp.unmatched, frame);
    }

    frame.setFailed();
    frame.addHelp(tokenMatcher, !tmp.matched.empty() ? MatchTypeEnum::Partial : MatchTypeEnum::Fail);
    return syntaxRecurseNext(node, input, frame);
}

ParseResult TreeParser::HelpCommon::syntaxRecurseNext(const Sublist &node,
                                                      const ParserInput &input,
                                                      HelpFrame &frame)
{
    if (!m_isFull)
        if (frame.getFailed()) {
            if (!node.hasNextNode() || node.getNext())
                frame.addHelp("...");
            return ParseResult::failure(input);
        }

    if (!node.hasNextNode())
        return recurseAccept(node, input, frame);

    if (const SharedConstSublist &tmp = node.getNext())
        return syntaxRecurseFirst(deref(tmp), input, frame);

    return ParseResult::failure(input);
}

ParseResult TreeParser::HelpCommon::recurseAccept(const Sublist &node,
                                                  const ParserInput &input,
                                                  HelpFrame &frame)
{
    if (!frame.getFailed() && !input.empty()) {
        frame.setFailed();
    }

    frame.addAccept(node.getAcceptFn().getHelp());
    return ParseResult::failure(input);
}

void TreeParser::help(const ParserInput &input, const bool isFull)
{
    const Sublist &node = deref(m_syntaxRoot);
    HelpFrame frame{m_user.getOstream()};
    MAYBE_UNUSED const ParseResult ignored = //
        HelpCommon{isFull}.syntaxRecurseFirst(node, input, frame);
}

std::string processSyntax(const syntax::SharedConstSublist &syntax,
                          const std::string &name,
                          const StringView args)
{
    using namespace syntax;

    const auto str = name + " " + args.toStdString();
    const auto cmd = StringView{str};
    auto v = unquote(cmd.getStdStringView(), true, false);
    if (!v) {
        throw std::runtime_error("input error: " + v.getUnquoteFailureReason());
    }

    const auto shared_vec = std::make_shared<const std::vector<std::string>>(v.getVectorOfStrings());

    const ParserInput input(shared_vec);
    std::ostringstream ss;
    User u{ss};
    TreeParser parser{syntax, u};

    {
        // REVISIT: check return value?
        MAYBE_UNUSED const auto ignored = //
            parser.parse(input);
    }

    return ss.str();
}

} // namespace syntax
