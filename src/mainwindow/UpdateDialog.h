#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/macros.h"
#include "UpdateChecker.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>

class NODISCARD_QOBJECT UpdateDialog : public QDialog
{
    Q_OBJECT

private:
    UpdateChecker m_checker;
    QLabel *m_text = nullptr;
    QDialogButtonBox *m_buttonBox = nullptr;

public:
    explicit UpdateDialog(QWidget *parent);

    void open() override;
};
