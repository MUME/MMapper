#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include <functional>
#include <memory>

#include <QFontDialog>

class QFont;
class QMenu;
class QMessageBox;
class QObject;
class QPoint;
class QWidget;
class QString;

namespace mmqt {
// recursively disconnect all children
extern void rdisconnect(QObject *obj);
extern void setWindowTitle2(QWidget &widget, const QString &program, const QString &title);
// Shows a window maximized when its preferred size would not fit the
// screen it is on (phone-sized screens), and normally otherwise.
extern void showFittedToScreen(QWidget &widget);
// Non-blocking replacements for the QMessageBox::information/warning/critical
// statics: the box is window-modal, deletes itself when closed, and never
// runs a nested event loop (which is unavailable on wasm without Asyncify).
// The returned box can be used to connect to QDialog::finished.
extern QMessageBox &showInformation(QWidget *parent, const QString &title, const QString &text);
extern QMessageBox &showWarning(QWidget *parent, const QString &title, const QString &text);
extern QMessageBox &showCritical(QWidget *parent, const QString &title, const QString &text);
// Non-blocking replacement for QMenu::exec(): the menu is shown at a global
// position and deletes itself when closed.
extern void popupMenu(std::unique_ptr<QMenu> menu, const QPoint &globalPos);
// Non-blocking replacement for QFontDialog::getFont(): onAccepted is only
// called if the user picks a font. Always uses Qt's own dialog, because the
// native mac font panel doesn't list fonts added with addApplicationFont().
extern QFontDialog &showFontDialog(QWidget *parent,
                                   const QFont &initial,
                                   const QString &title,
                                   QFontDialog::FontDialogOptions options,
                                   std::function<void(const QFont &)> onAccepted);
} // namespace mmqt
