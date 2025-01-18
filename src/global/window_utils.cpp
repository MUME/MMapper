// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "window_utils.h"

#include "utils.h"

#include <QString>
#include <QWidget>

// NOLINTNEXTLINE (yes, recursion is the entire point)
void mmqt::rdisconnect(QObject *const obj)
{
    auto &o = deref(obj);
    for (QObject *const child : o.children()) {
        rdisconnect(child);
    }
    o.disconnect();
}

void mmqt::setWindowTitle2(QWidget &widget, const QString &program, const QString &title)
{
    // Maybe this should be a global config option?
    static const auto programFirst = utils::getEnvBool("MMAPPER_WINDOW_TITLE_PROGRAM_FIRST")
                                         .value_or(false);

    // Many programs show "filename - program", and that works well in some cases,
    // but in MMapper's case, the user does not have any control over the title
    // of a remote edit / view, so the message shown in the taskbar can be confusing.
    if (programFirst) {
        // Choosing program first gives "program - filename", which at least lets the user
        // see that it's a mmapper window:
        // "MMapper - untitled"
        // "MMapper Editor - Write your message to Gandalf."
        // "MMapper Viewer - View text..."
        widget.setWindowTitle(QString("%1 - %2").arg(program, title));
    } else {
        // "untitled - MMapper"
        // "Write your message to Gandalf. - MMapper Editor"
        // "View text... - MMapper Viewer"
        widget.setWindowTitle(QString("%1 - %2").arg(title, program));
    }
}
