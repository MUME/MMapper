#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/macros.h"

#include <QString>
#include <QWidget>
#include <QtCore>

class QObject;

namespace Ui {
class MumeProtocolPage;
}

class NODISCARD_QOBJECT MumeProtocolPage final : public QWidget
{
    Q_OBJECT

private:
    Ui::MumeProtocolPage *const ui;

public:
    explicit MumeProtocolPage(QWidget *parent);
    ~MumeProtocolPage() final;

public slots:
    void slot_loadConfig();
    void slot_internalEditorRadioButtonChanged(bool);
    void slot_externalEditorCommandTextChanged(QString);
    void slot_externalEditorBrowseButtonClicked(bool);
    void slot_gmcpBroadcastCheckBoxChanged(int);
    void slot_gmcpIntervalSpinBoxChanged(int);
};
