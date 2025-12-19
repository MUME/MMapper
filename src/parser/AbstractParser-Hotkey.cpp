// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../configuration/configuration.h"
#include "../global/TextUtils.h"
#include "../syntax/SyntaxArgs.h"
#include "../syntax/TreeParser.h"
#include "AbstractParser-Utils.h"
#include "abstractparser.h"

#include <ostream>
#include <sstream>
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
    return os << "<key>";
}

void AbstractParser::parseHotkey(StringView input)
{
    using namespace ::syntax;
    static const auto abb = syntax::abbrevToken;

    // _hotkey set KEY command
    auto setHotkey = Accept(
        [](User &user, const Pair *const args) {
            auto &os = user.getOstream();
            const auto v = getAnyVectorReversed(args);

            const auto keyName = mmqt::toQStringUtf8(v[1].getString());
            const std::string cmdStr = concatenate_unquoted(v[2].getVector());
            const auto command = mmqt::toQStringUtf8(cmdStr);

            setConfig().hotkeyManager.setHotkey(keyName, command);
            os << "Hotkey set: " << mmqt::toStdStringUtf8(keyName.toUpper()) << " = " << cmdStr
               << "\n";
            send_ok(os);
        },
        "set hotkey");

    // _hotkey remove KEY
    auto removeHotkey = Accept(
        [](User &user, const Pair *const args) {
            auto &os = user.getOstream();
            const auto v = getAnyVectorReversed(args);

            const auto keyName = mmqt::toQStringUtf8(v[1].getString());

            if (getConfig().hotkeyManager.hasHotkey(keyName)) {
                setConfig().hotkeyManager.removeHotkey(keyName);
                os << "Hotkey removed: " << mmqt::toStdStringUtf8(keyName.toUpper()) << "\n";
            } else {
                os << "No hotkey configured for: " << mmqt::toStdStringUtf8(keyName.toUpper())
                   << "\n";
            }
            send_ok(os);
        },
        "remove hotkey");

    // _hotkey config (list all)
    auto listHotkeys = Accept(
        [](User &user, const Pair *) {
            auto &os = user.getOstream();
            const auto &hotkeys = getConfig().hotkeyManager.getAllHotkeys();

            if (hotkeys.empty()) {
                os << "No hotkeys configured.\n";
            } else {
                os << "Configured hotkeys:\n";
                for (const auto &[key, cmd] : hotkeys) {
                    // key is QString (needs conversion), cmd is already std::string
                    os << "  " << mmqt::toStdStringUtf8(key) << " = " << cmd << "\n";
                }
            }
            send_ok(os);
        },
        "list hotkeys");

    // _hotkey keys (show available keys)
    auto listKeys = Accept(
        [](User &user, const Pair *) {
            auto &os = user.getOstream();
            os << "Available key names:\n"
               << "  Function keys: F1-F12\n"
               << "  Numpad: NUMPAD0-9, NUMPAD_SLASH, NUMPAD_ASTERISK,\n"
               << "          NUMPAD_MINUS, NUMPAD_PLUS, NUMPAD_PERIOD\n"
               << "  Navigation: HOME, END, INSERT, PAGEUP, PAGEDOWN\n"
               << "  Arrow keys: UP, DOWN, LEFT, RIGHT\n"
               << "  Misc: ACCENT, 0-9, HYPHEN, EQUAL\n"
               << "\n"
               << "Available modifiers: CTRL, SHIFT, ALT, META\n"
               << "\n"
               << "Examples: CTRL+F1, SHIFT+NUMPAD8, ALT+F5\n";
            send_ok(os);
        },
        "list available keys");

    // _hotkey reset
    auto resetHotkeys = Accept(
        [](User &user, const Pair *) {
            auto &os = user.getOstream();
            setConfig().hotkeyManager.resetToDefaults();
            os << "Hotkeys reset to defaults.\n";
            send_ok(os);
        },
        "reset to defaults");

    // Build syntax tree
    auto setSyntax = buildSyntax(abb("set"),
                                 TokenMatcher::alloc<ArgHotkeyName>(),
                                 TokenMatcher::alloc<ArgRest>(),
                                 setHotkey);

    auto removeSyntax = buildSyntax(abb("remove"),
                                    TokenMatcher::alloc<ArgHotkeyName>(),
                                    removeHotkey);

    auto configSyntax = buildSyntax(abb("config"), listHotkeys);

    auto keysSyntax = buildSyntax(abb("keys"), listKeys);

    auto resetSyntax = buildSyntax(abb("reset"), resetHotkeys);

    auto hotkeyUserSyntax = buildSyntax(setSyntax,
                                        removeSyntax,
                                        configSyntax,
                                        keysSyntax,
                                        resetSyntax);

    eval("hotkey", hotkeyUserSyntax, input);
}
