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
    // "untitled - MMapper"
    // "Write your message to Gandalf. - MMapper Editor"
    // "View text... - MMapper Viewer"
    widget.setWindowTitle(QString("%1 - %2").arg(title, program));
}
