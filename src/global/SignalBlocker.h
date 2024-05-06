#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "RuleOf5.h"
#include "utils.h"

#include <QObject>
#include <QVector>

class QWidget;

// Less error-prone than QSignalBlocker, because it forbids discarding, copying, and moving.
// NOTE: This is not related to Signal<T>, which is a simpler replacement for QT signals.
class NODISCARD SignalBlocker final
{
private:
    QObject &obj;
    const bool wasBlocked;

public:
    explicit SignalBlocker(QObject &in);
    ~SignalBlocker();
    DELETE_CTORS_AND_ASSIGN_OPS(SignalBlocker);
};
