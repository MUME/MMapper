// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "RemoteEditDocumentOps.h"

#include "../global/AnsiTextUtils.h"
#include "../global/Consts.h"
#include "../global/LineUtils.h"
#include "../global/TabUtils.h"
#include "../global/TextBuffer.h"
#include "../global/TextUtils.h"

#include <algorithm>
#include <cassert>
#include <functional>
#include <sstream>

#include <QRegularExpression>
#include <QTextBlock>

using namespace char_consts;

namespace mmqt {

int measureTabAndAnsiAware(const QString &s)
{
    int col = 0;
    for (const AnsiStringToken token : mmqt::AnsiTokenizer{s}) {
        using Type = decltype(token.type);
        switch (token.type) {
        case Type::Ansi:
            break;
        case Type::Newline:
            /* unexpected */
            col = 0;
            break;
        case Type::Space:
        case Type::Control:
        case Type::Word:
            col = mmqt::measureExpandedTabsOneLine(token.getQStringView(), col);
            break;
        }
    }
    return col;
}

bool remoteEditLineHasTabs(const QTextCursor &line)
{
    if (line.isNull()) {
        return false;
    }
    const auto &block = line.block();
    if (!block.isValid()) {
        return false;
    }
    return block.text().indexOf(C_TAB) >= 0;
}

void remoteEditExpandTabsOnLine(QTextCursor line)
{
    const QTextBlock &block = line.block();
    const QString &s = block.text();

    TextBuffer prefix;
    prefix.appendExpandedTabs(QStringView{s}, 0);

    line.setPosition(block.position());
    line.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
    line.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    line.insertText(prefix.getQString());
}

void remoteEditTryRemoveLeadingSpaces(QTextCursor line, const int max_spaces)
{
    const auto &block = line.block();
    if (!block.isValid()) {
        return;
    }

    const auto &text = block.text();
    if (text.isEmpty()) {
        return;
    }

    const int to_remove = std::invoke([&text, max_spaces]() -> int {
        const int len = std::min(max_spaces, static_cast<int>(text.length()));
        int n = 0;
        while (n < len && text.at(n) == C_SPACE) {
            ++n;
        }
        return n;
    });

    if (to_remove == 0) {
        return;
    }

    line.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
    line.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, to_remove);
    line.removeSelectedText();
}

void remoteEditInsertPrefix(QTextCursor line, const QString &prefix)
{
    line.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
    line.insertText(prefix);
}

void remoteEditPrefixPartialSelection(QTextCursor cur, const QString &prefix)
{
    RemoteEditUndoGroup raii(cur);
    remoteEditForeachPartlySelectedBlock(cur, [&prefix](const QTextCursor &line) -> void {
        if (remoteEditLineHasTabs(line)) {
            remoteEditExpandTabsOnLine(line);
        }
        remoteEditInsertPrefix(line, prefix);
    });
}

void remoteEditUnindentPartialSelection(QTextCursor cur)
{
    RemoteEditUndoGroup raii(cur);
    remoteEditForeachPartlySelectedBlock(cur, [](const QTextCursor &line) -> void {
        if (remoteEditLineHasTabs(line)) {
            remoteEditExpandTabsOnLine(line);
        }
        remoteEditTryRemoveLeadingSpaces(line, 2);
    });
}

void remoteEditJoinLines(QTextCursor cur)
{
    RemoteEditUndoGroup raii(cur);

    auto *const doc = cur.document();
    const auto from = doc->findBlock(cur.selectionStart());
    auto to = doc->findBlock(cur.selectionEnd());

    /* Feature: join the next line if only one line is selected */
    if (from == to) {
        if (cur.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor)) {
            to = doc->findBlock(cur.selectionEnd());
        }
    }

    TextBuffer buffer;
    remoteEditForeachPartlySelectedBlock(cur, [&buffer](const QTextCursor line) -> void {
        const QTextBlock block = line.block();
        const QString text = block.text();
        if (text.isEmpty()) {
            return;
        }

        if (!buffer.isEmpty()) {
            buffer.append(C_SPACE);
        }
        buffer.appendExpandedTabs(QStringView{text});
    });

    /* adjust the selection to cover select everything in the blocks */
    const int a = from.position();
    const int b = to.position() + to.length() - 1;
    cur.setPosition(a);
    cur.setPosition(b, QTextCursor::KeepAnchor);
    cur.insertText(buffer.getQString());
}

void remoteEditJustifyLines(QTextCursor cur, const int maxLength)
{
    RemoteEditUndoGroup raii(cur);

    auto *const doc = cur.document();
    const auto from = doc->findBlock(cur.selectionStart());
    const auto to = doc->findBlock(cur.selectionEnd());
    const auto toBlockNumber = to.blockNumber();
    if (!from.isValid()) {
        return;
    }

    const int a = from.position();
    const int b = to.position() + to.length() - 1;

    TextBuffer buffer;
    for (auto it = from; it.isValid() && it.blockNumber() <= toBlockNumber; it = it.next()) {
        const auto &text = it.text();
        if (text.isEmpty()) {
            continue;
        }

        if (!buffer.isEmpty()) {
            buffer.append(C_SPACE);
        }
        buffer.appendJustified(QStringView{text}, maxLength);
    }

    cur.setPosition(a);
    cur.setPosition(b, QTextCursor::KeepAnchor);
    cur.insertText(buffer.getQString());
}

QString remoteEditJustifyText(const QString &text, const int maxLength)
{
    TextBuffer result;
    result.reserve(static_cast<int>(text.length()));
    mmqt::foreachLine(text, [&result, maxLength](const QStringView line, bool /*hasNewline*/) {
        result.appendJustified(line, maxLength);
        result.append(C_NEWLINE);
    });
    return result.getQString();
}

void remoteEditExpandTabs(QTextCursor cur)
{
    RemoteEditUndoGroup raii(cur);
    remoteEditForeachPartlySelectedBlock(cur, [](QTextCursor line) -> void {
        if (remoteEditLineHasTabs(line)) {
            remoteEditExpandTabsOnLine(line);
        }
    });
}

void remoteEditRemoveTrailingWhitespace(QTextCursor cur)
{
    RemoteEditUndoGroup raii(cur);
    remoteEditForeachPartlySelectedBlock(cur, [](QTextCursor line) -> void {
        static const QRegularExpression trailingWhitespace(R"([[:space:]]+$)");
        assert(trailingWhitespace.isValid());
        QString text = line.block().text();
        if (!text.contains(trailingWhitespace)) {
            return;
        }

        text.remove(trailingWhitespace);
        line.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
        line.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        line.insertText(text);
    });
}

void remoteEditRemoveDuplicateSpaces(QTextCursor cur)
{
    RemoteEditUndoGroup raii(cur);
    remoteEditForeachPartlySelectedBlock(cur, [](QTextCursor line) -> void {
        static const QRegularExpression spaces(R"((\t|[[:space:]]{2,}))");
        assert(spaces.isValid());
        QString text = line.block().text();
        if (!text.contains(spaces)) {
            return;
        }

        text.replace(spaces, string_consts::S_SPACE);
        line.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
        line.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        line.insertText(text);
    });
}

std::optional<QString> remoteEditNormalizeAnsi(const QString &text)
{
    if (!mmqt::containsAnsi(text)) {
        return std::nullopt;
    }

    TextBuffer output = mmqt::normalizeAnsi(ANSI_COLOR_SUPPORT_HI, text);
    if (!output.hasTrailingNewline()) {
        output.append(C_NEWLINE);
    }

    return output.getQString();
}

RemoteEditFindOutcome remoteEditFind(QTextDocument &doc,
                                     const QTextCursor &cursor,
                                     const QString &term,
                                     const QTextDocument::FindFlags flags)
{
    RemoteEditFindOutcome outcome;
    outcome.cursor = cursor;
    if (term.isEmpty()) {
        return outcome;
    }

    if (QTextCursor found = doc.find(term, cursor, flags); !found.isNull()) {
        outcome.result = RemoteEditFindResultEnum::FOUND;
        outcome.cursor = found;
        return outcome;
    }

    // Wrap around
    QTextCursor wrapStart(&doc);
    wrapStart.movePosition(flags.testFlag(QTextDocument::FindBackward) ? QTextCursor::End
                                                                       : QTextCursor::Start);
    if (QTextCursor found = doc.find(term, wrapStart, flags); !found.isNull()) {
        outcome.result = RemoteEditFindResultEnum::FOUND_AFTER_WRAP;
        outcome.cursor = found;
        return outcome;
    }

    return outcome;
}

bool remoteEditReplaceCurrentIfMatches(QTextCursor &cursor,
                                       const QString &findTerm,
                                       const QString &replaceTerm,
                                       const QTextDocument::FindFlags flags)
{
    if (!cursor.hasSelection()) {
        return false;
    }

    const QString selectedText = cursor.selectedText();
    const Qt::CaseSensitivity cs = flags.testFlag(QTextDocument::FindCaseSensitively)
                                       ? Qt::CaseSensitive
                                       : Qt::CaseInsensitive;
    if (QString::compare(selectedText, findTerm, cs) != 0) {
        return false;
    }

    cursor.insertText(replaceTerm);
    return true;
}

int remoteEditReplaceAll(QTextDocument &doc,
                         const QString &findTerm,
                         const QString &replaceTerm,
                         const QTextDocument::FindFlags flags)
{
    if (findTerm.isEmpty()) {
        return 0;
    }

    int replacements = 0;
    QTextCursor undoCursor(&doc);
    RemoteEditUndoGroup raii(undoCursor);

    QTextCursor pos(&doc);
    for (QTextCursor found = doc.find(findTerm, pos, flags); !found.isNull();
         found = doc.find(findTerm, found, flags)) {
        found.insertText(replaceTerm);
        ++replacements;
    }

    return replacements;
}

std::optional<QTextCursor> remoteEditGotoLine(QTextDocument &doc, const int lineNum)
{
    QTextCursor cursor(&doc);
    cursor.movePosition(QTextCursor::Start);
    if (cursor.movePosition(QTextCursor::NextBlock, QTextCursor::MoveAnchor, lineNum - 1)) {
        return cursor;
    }
    return std::nullopt;
}

namespace { // anonymous

struct NODISCARD CursorColumnInfo final
{
    int actual = 0;
    int tab_aware = 0;
    int tab_and_ansi_aware = 0;
};

NODISCARD CursorColumnInfo getCursorColumn(QTextCursor &cursor)
{
    const int pos = cursor.positionInBlock();
    const auto &block = cursor.block();
    const auto &text = block.text().left(pos);

    CursorColumnInfo cci;
    cci.actual = pos;
    cci.tab_aware = mmqt::measureExpandedTabsOneLine(text, 0);
    cci.tab_and_ansi_aware = mmqt::measureTabAndAnsiAware(text);
    return cci;
}

struct NODISCARD CursorAnsiInfo final
{
    TextBuffer buffer;

    explicit operator bool() const { return !buffer.isEmpty(); }
};

NODISCARD CursorAnsiInfo getCursorAnsi(QTextCursor cursor)
{
    const int pos = cursor.positionInBlock();

    CursorAnsiInfo result;
    const auto &line = cursor.block().text();
    mmqt::foreachAnsi(line, [pos, &result](auto start, const QStringView sv) {
        if (result || pos < start || pos >= start + sv.length()) {
            return;
        }

        if (!mmqt::isValidAnsiColor(sv)) {
            result.buffer.append("*invalid*");
            return;
        }

        auto &buffer = result.buffer;

        if (const auto optAnsi = mmqt::parseAnsiColor(RawAnsi{}, sv)) {
            const auto &ansi = *optAnsi;
            if (ansi == RawAnsi{}) {
                buffer.append("reset");
            } else {
                std::ostringstream oss;
                to_stream(oss, ansi);
                buffer.append(mmqt::toQStringUtf8(oss.str()));
            }
        } else {
            result.buffer.append("*error");
        }
    });
    return result;
}

NODISCARD bool linesHaveTabs(const QTextCursor &cur)
{
    return remoteEditExistsPartlySelectedBlock(cur, [](const QTextCursor &it) -> bool {
        return remoteEditLineHasTabs(it);
    });
}

NODISCARD bool linesHaveTrailingSpace(const QTextCursor &cur)
{
    return remoteEditExistsPartlySelectedBlock(cur, [](const QTextCursor &it) -> bool {
        const auto &block = it.block();
        const auto &line = block.text();
        return mmqt::findTrailingWhitespace(line) >= 0;
    });
}

NODISCARD bool hasLongLines(const QTextCursor &cur)
{
    return remoteEditExistsPartlySelectedBlock(cur, [](const QTextCursor &line) -> bool {
        const auto &block = line.block();
        const auto &s = block.text();
        return mmqt::measureTabAndAnsiAware(s) > 80;
    });
}

} // namespace

RemoteEditStatusInfo computeRemoteEditStatus(const QTextCursor &cursorIn)
{
    QTextCursor cur = cursorIn;
    RemoteEditStatusInfo info;

    const auto cci = getCursorColumn(cur);
    info.line = cur.blockNumber() + 1;
    info.col = cci.tab_and_ansi_aware + 1;
    if (cci.tab_aware != cci.tab_and_ansi_aware) {
        info.nonAnsiAwareCol = cci.tab_aware + 1;
    }

    if (cur.hasSelection()) {
        const QString selection = cur.selection().toPlainText();
        RemoteEditSelectionInfo sel;
        sel.chars = static_cast<int>(selection.length());
        sel.lines = static_cast<int>(selection.count(C_NEWLINE)
                                     + (selection.endsWith(C_NEWLINE) ? 0 : 1));
        info.selection = sel;
    } else if (auto ansi = getCursorAnsi(cur)) {
        info.ansiCode = ansi.buffer.getQString();
    }

    QTextCursor errCur = cur;
    if (!errCur.hasSelection()) {
        errCur.select(QTextCursor::Document);
    }
    info.hasTabs = linesHaveTabs(errCur);
    info.hasTrailingSpace = linesHaveTrailingSpace(errCur);
    info.hasLongLines = hasLongLines(errCur);

    return info;
}

} // namespace mmqt
