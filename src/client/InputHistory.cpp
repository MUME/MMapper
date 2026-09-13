// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "InputHistory.h"

#include "../configuration/configuration.h"

#include <QRegularExpression>

static constexpr const int MIN_WORD_LENGTH = 3;

static const QRegularExpression g_whitespaceRx(R"(\s+)");

void InputHistory::addInputLine(const QString &string)
{
    if (!string.isEmpty() && (empty() || back() != string)) {
        // Add to line history if it is a new entry
        push_front(string);
    }

    // Trim line history
    if (static_cast<int>(size()) > getConfig().integratedClient.linesOfInputHistory) {
        pop_back();
    }

    // Reset the iterator
    m_iterator = begin();
}

void TabHistory::addInputLine(const QString &string)
{
    QStringList list = string.split(g_whitespaceRx, Qt::SplitBehaviorFlags::SkipEmptyParts);
    for (const QString &word : list) {
        if (word.length() > MIN_WORD_LENGTH) {
            // Adding this word to the dictionary
            push_front(word);

            // Trim dictionary
            if (static_cast<int>(size())
                > getConfig().integratedClient.tabCompletionDictionarySize) {
                pop_back();
            }
        }
    }

    // Reset the iterator
    m_iterator = begin();
}

std::optional<QString> TabHistory::nextMatch(const QString &fragment)
{
    while (!atEnd()) {
        const QString word = value();
        forward();
        if (!word.startsWith(fragment)) {
            continue;
        }
        if (atEnd()) {
            // The match found was the last entry in the dictionary: mirrors
            // InputWidget::tabComplete()'s original two-step
            // apply-then-immediately-revert behavior for this case (the
            // completion it would have applied gets undone by the very next
            // "reached end" check), so the net effect is the same as no
            // match having been found at all.
            reset();
            return std::nullopt;
        }
        return word;
    }
    reset();
    return std::nullopt;
}
