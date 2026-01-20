// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "../client/Hotkey.h"
#include "../client/HotkeyManager.h"
#include "../syntax/SyntaxArgs.h"
#include "../syntax/TreeParser.h"
#include "AbstractParser-Utils.h"
#include "abstractparser.h"

#include <algorithm>
#include <map>
#include <ostream>
#include <vector>

class NODISCARD ArgHotkeyName final : public syntax::IArgument
{
private:
    NODISCARD syntax::MatchResult virt_match(const syntax::ParserInput &input,
                                             syntax::IMatchErrorLogger *) const final;

    std::ostream &virt_to_stream(std::ostream &os) const final;
};

syntax::MatchResult ArgHotkeyName::virt_match(const syntax::ParserInput &input,
                                              syntax::IMatchErrorLogger * /*logger */) const
{
    if (input.empty()) {
        return syntax::MatchResult::failure(input);
    }

    return syntax::MatchResult::success(1, input, Value{input.front()});
}

std::ostream &ArgHotkeyName::virt_to_stream(std::ostream &os) const
{
    return os << "<key-combination>";
}

namespace {
void getInstructionalError(AnsiOstream &os, std::string_view keyName)
{
    os << "Invalid key combination: [" << keyName << "]\n";

    {
        os << "\nValid modifiers:\n  ";
        const auto mods = {
#define X_STR(id, camel, qtenum) #id,
            XFOREACH_HOTKEY_MODIFIER(X_STR)
#undef X_STR
        };
        bool first = true;
        for (const auto &mod : mods) {
            if (!first)
                os << ", ";
            os << mod;
            first = false;
        }
    }

    {
        os << "\nValid keys:\n  ";
        std::map<std::string, HotkeyPolicyEnum> keys;
#define X_SORT(EnumName, StringName, QtKey, Policy) keys[StringName] = Policy;
        XFOREACH_HOTKEY_BASE_KEYS(X_SORT)
#undef X_SORT
        bool first = true;
        for (const auto &[name, policy] : keys) {
            if (!first)
                os << ", ";

            os << name;
#define X_MARKER(EnumName, Marker, Help) \
    if (policy == HotkeyPolicyEnum::EnumName) { \
        os << Marker; \
    } else
            XFOREACH_HOTKEY_POLICY(X_MARKER)
#undef X_MARKER
            {}
            first = false;
        }
        os << "\n";
#define X_FOOTNOTE(EnumName, Marker, Help) \
    if (std::string_view(Marker).length() > 0) { \
        os << "\n" << Marker << " = " << Help; \
    }
        XFOREACH_HOTKEY_POLICY(X_FOOTNOTE)
#undef X_FOOTNOTE
        os << "\n";
    }

    os << "\nExamples:  NUMPAD4  SHIFT+NUMPAD4  CTRL+META+0\n";
}
} // namespace

void AbstractParser::parseHotkey(StringView input)
{
    using namespace ::syntax;
    static const auto abb = syntax::abbrevToken;

    auto bindHotkey = Accept(
        [this](User &user, const Pair *const args) {
            auto &os = user.getOstream();
            const auto v = getAnyVectorReversed(args);

            const std::string keyName = v[1].getString();

            Hotkey hk(keyName);
            if (!hk.isRecognized()) {
                getInstructionalError(os, keyName);
                return;
            }

            if (!hk.isPolicySatisfied()) {
#define X_POLICY_ERROR(EnumName, Marker, Help) \
    if (hk.is##EnumName()) { \
        os << "Error: [" << keyName << "] " << Help << ".\n"; \
    } else
                XFOREACH_HOTKEY_POLICY(X_POLICY_ERROR)
#undef X_POLICY_ERROR
                {}
                return;
            }

            const std::string cmdStr = v[2].isVector() ? concatenate_unquoted(v[2].getVector())
                                                       : v[2].getString();
            if (cmdStr.empty()) {
                os << "No command provided.\n";
                return;
            }

            if (m_hotkeyManager.setHotkey(hk, cmdStr)) {
                os << "Hotkey bound: [" << hk.to_string() << "] -> " << cmdStr << "\n";
                send_ok(os);
            } else {
                os << "Failed to bind hotkey.\n";
            }
        },
        "bind hotkey");

    auto listHotkeys = Accept(
        [this](User &user, const Pair *) {
            auto &os = user.getOstream();
            auto hotkeys = m_hotkeyManager.getAllHotkeys();
            std::sort(hotkeys.begin(), hotkeys.end(), [](const auto &a, const auto &b) {
                return a.first.to_string() < b.first.to_string();
            });

            if (hotkeys.empty()) {
                os << "No hotkeys configured.\n";
            } else {
                os << "Active Hotkeys:\n";
                for (const auto &[hk, cmd] : hotkeys) {
                    os << "  [" << hk.to_string() << "] -> " << cmd << "\n";
                }
                os << "Total: " << hotkeys.size() << "\n";
            }
            send_ok(os);
        },
        "list hotkeys");

    auto unbindHotkey = Accept(
        [this](User &user, const Pair *const args) {
            auto &os = user.getOstream();
            const auto v = getAnyVectorReversed(args);
            const std::string keyName = v[1].getString();

            Hotkey hk{keyName};
            if (!hk.isRecognized()) {
                getInstructionalError(os, keyName);
                return;
            }

            if (m_hotkeyManager.removeHotkey(hk)) {
                os << "Hotkey unbound: [" << hk.to_string() << "]\n";
                send_ok(os);
            } else {
                os << "No hotkey configured for: [" << hk.to_string() << "]\n";
            }
        },
        "unbind hotkey");

    // Build syntax tree
    auto bindSyntax = buildSyntax(abb("bind"),
                                  TokenMatcher::alloc<ArgHotkeyName>(),
                                  TokenMatcher::alloc<ArgRest>(),
                                  bindHotkey);

    auto listSyntax = buildSyntax(abb("list"), listHotkeys);

    auto unbindSyntax = buildSyntax(abb("unbind"),
                                    TokenMatcher::alloc<ArgHotkeyName>(),
                                    unbindHotkey);

    auto hotkeyUserSyntax = buildSyntax(bindSyntax, listSyntax, unbindSyntax);

    eval("hotkey", hotkeyUserSyntax, input);
}
