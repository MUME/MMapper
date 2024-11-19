#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

class QObject;
class QWidget;
class QString;

namespace mmqt {
// recursively disconnect all children
extern void rdisconnect(QObject *obj);
extern void setWindowTitle2(QWidget &widget, const QString &program, const QString &title);
} // namespace mmqt
