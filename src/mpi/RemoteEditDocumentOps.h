#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/macros.h"

#include <optional>
#include <type_traits>

#include <QString>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextDocumentFragment>

// Widget-free core of the MPI remote editor's text operations, find/replace, goto-line,
// and status-bar computation. Everything here operates on QTextDocument/QTextCursor
// (QtGui), so it has no dependency on the QWidget-based RemoteEditWidget.
namespace mmqt {

// REVISIT: Figure out how to tweak logic to accept actual maximum length of 80
inline constexpr int REMOTE_EDIT_MAX_LENGTH = 79;

NODISCARD int measureTabAndAnsiAware(const QString &s);

/// Groups everything done in its scope into a single undo action on the cursor's document.
class NODISCARD RemoteEditUndoGroup final
{
private:
    QTextCursor m_cursor;

public:
    explicit RemoteEditUndoGroup(const QTextCursor &cursor)
        : m_cursor{cursor}
    {
        m_cursor.beginEditBlock();
    }
    ~RemoteEditUndoGroup() { m_cursor.endEditBlock(); }
};

struct NODISCARD RemoteEditLineRange final
{
    NODISCARD static QTextCursor beg(QTextCursor cur)
    {
        auto from = cur.document()->findBlock(cur.selectionStart());
        return QTextCursor(from);
    }

    NODISCARD static QTextCursor end(QTextCursor cur)
    {
        const auto &to = cur.document()->findBlock(cur.selectionEnd());
        auto endCursor = QTextCursor(to);
        if (!endCursor.isNull()) {
            endCursor.movePosition(QTextCursor::NextBlock);
        }
        return endCursor;
    }
};

/// Caller is responsible for not invalidating the end cursor.
///
/// callback(QTextCursor);
/// -or-
/// callback(const QTextCursor&);
template<typename Callback>
void remoteEditForeachPartlySelectedBlock(QTextCursor cur, Callback &&callback)
{
    auto beg = RemoteEditLineRange::beg(cur);
    auto end = RemoteEditLineRange::end(cur);
    for (auto it = beg; !it.isNull() && it <= end;) {
        if (it.block().isValid()) {
            callback(static_cast<const QTextCursor &>(it));
        }
        if (!it.movePosition(QTextCursor::NextBlock)) {
            break;
        }
    }
}

enum class NODISCARD RemoteEditCallbackResultEnum { KEEP_GOING, STOP };

/// Caller is responsible for not invalidating the end cursor.
///
/// RemoteEditCallbackResultEnum callback(QTextCursor);
/// -or-
/// RemoteEditCallbackResultEnum callback(const QTextCursor&);
template<typename Callback>
    requires(std::is_invocable_r_v<RemoteEditCallbackResultEnum, Callback, const QTextCursor &>)
void remoteEditForeachPartlySelectedBlockUntil(QTextCursor cur, Callback &&callback)
{
    auto beg = RemoteEditLineRange::beg(cur);
    auto end = RemoteEditLineRange::end(cur);
    for (auto it = beg; !it.isNull() && it < end;) {
        if (it.block().isValid()) {
            if (callback(static_cast<const QTextCursor &>(it))
                == RemoteEditCallbackResultEnum::STOP) {
                return;
            }
        }
        if (!it.movePosition(QTextCursor::NextBlock)) {
            break;
        }
    }
}

template<typename Callback>
    requires(std::is_invocable_r_v<bool, Callback, const QTextCursor &>)
bool remoteEditExistsPartlySelectedBlock(QTextCursor cur, Callback &&callback)
{
    bool result = false;
    remoteEditForeachPartlySelectedBlockUntil(cur,
                                              [&result, &callback](
                                                  QTextCursor it) -> RemoteEditCallbackResultEnum {
                                                  if (callback(it)) {
                                                      result = true;
                                                      return RemoteEditCallbackResultEnum::STOP;
                                                  }
                                                  return RemoteEditCallbackResultEnum::KEEP_GOING;
                                              });
    return result;
}

NODISCARD bool remoteEditLineHasTabs(const QTextCursor &line);
void remoteEditExpandTabsOnLine(QTextCursor line);
void remoteEditTryRemoveLeadingSpaces(QTextCursor line, int max_spaces);
void remoteEditInsertPrefix(QTextCursor line, const QString &prefix);

// --- Text operations (each groups its edits into a single undo action) ---

/// 2-space block indentation of all partly-selected lines (tabs on those lines are
/// expanded first).
void remoteEditPrefixPartialSelection(QTextCursor cur, const QString &prefix);

/// 2-space block de-indentation of all partly-selected lines (tabs on those lines are
/// expanded first).
void remoteEditUnindentPartialSelection(QTextCursor cur);

/// Join all partly-selected lines into one (joining the next line too if only one line
/// is selected), replacing tabs with their expansion and separating lines with a space.
void remoteEditJoinLines(QTextCursor cur);

/// Justify all partly-selected lines to maxLength columns.
void remoteEditJustifyLines(QTextCursor cur, int maxLength);

/// Justify the entire document to maxLength columns; returns the new document text.
NODISCARD QString remoteEditJustifyText(const QString &text, int maxLength);

/// Expand tabs (to 8-character tabstops) on all partly-selected lines.
void remoteEditExpandTabs(QTextCursor cur);

/// Remove trailing whitespace from all partly-selected lines.
void remoteEditRemoveTrailingWhitespace(QTextCursor cur);

/// Collapse tabs and runs of 2+ whitespace characters to a single space on all
/// partly-selected lines.
void remoteEditRemoveDuplicateSpaces(QTextCursor cur);

/// Normalizes ANSI codes in the whole document; returns std::nullopt if the text
/// contains no ANSI codes at all (a no-op).
NODISCARD std::optional<QString> remoteEditNormalizeAnsi(const QString &text);

// --- Find / replace ---

enum class NODISCARD RemoteEditFindResultEnum { NOT_FOUND, FOUND, FOUND_AFTER_WRAP };

struct NODISCARD RemoteEditFindOutcome final
{
    RemoteEditFindResultEnum result = RemoteEditFindResultEnum::NOT_FOUND;
    // Valid selection cursor when result != NOT_FOUND; otherwise unset.
    QTextCursor cursor;
};

/// Finds the next occurrence of term starting from cursor; if not found, wraps around
/// to the start (or end, for a backward search) of the document and tries once more.
NODISCARD RemoteEditFindOutcome remoteEditFind(QTextDocument &doc,
                                               const QTextCursor &cursor,
                                               const QString &term,
                                               QTextDocument::FindFlags flags);

/// If cursor currently has a selection matching findTerm (per the case-sensitivity in
/// flags), replaces it with replaceTerm. Returns whether a replacement was made.
NODISCARD bool remoteEditReplaceCurrentIfMatches(QTextCursor &cursor,
                                                 const QString &findTerm,
                                                 const QString &replaceTerm,
                                                 QTextDocument::FindFlags flags);

/// Replaces every occurrence of findTerm in the whole document with replaceTerm, as a
/// single undo action. Returns the number of replacements made.
NODISCARD int remoteEditReplaceAll(QTextDocument &doc,
                                   const QString &findTerm,
                                   const QString &replaceTerm,
                                   QTextDocument::FindFlags flags);

// --- Goto line ---

/// Returns a cursor positioned at the start of 1-based lineNum, or std::nullopt if the
/// document doesn't have that many lines.
NODISCARD std::optional<QTextCursor> remoteEditGotoLine(QTextDocument &doc, int lineNum);

// --- Status bar ---

struct NODISCARD RemoteEditSelectionInfo final
{
    int chars = 0;
    int lines = 0;
};

struct NODISCARD RemoteEditStatusInfo final
{
    int line = 1;                       // 1-based
    int col = 1;                        // 1-based, tab+ansi aware
    std::optional<int> nonAnsiAwareCol; // 1-based; set only if it differs from col
    std::optional<RemoteEditSelectionInfo> selection;
    QString ansiCode; // only meaningful when !selection
    bool hasTabs = false;
    bool hasTrailingSpace = false;
    bool hasLongLines = false;
};

NODISCARD RemoteEditStatusInfo computeRemoteEditStatus(const QTextCursor &cursor);

} // namespace mmqt
