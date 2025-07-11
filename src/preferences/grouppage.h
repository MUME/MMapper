#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include "../global/macros.h"

#include <QColor>
#include <QWidget>

class QCheckBox;
class QPushButton;
class QLabel;
class QColorDialog;

namespace Ui {
class GroupPage;
}

class NODISCARD_QOBJECT GroupPage final : public QWidget
{
    Q_OBJECT

private:
    Ui::GroupPage *const ui;

public:
    explicit GroupPage(QWidget *parent = nullptr);
    ~GroupPage() final;

signals:
    void sig_groupSettingsChanged();
    void sig_showTokensChanged(bool);

public slots:
    void slot_loadConfig();

private slots:
    void slot_chooseColor();
    void slot_chooseNpcOverrideColor();
};
