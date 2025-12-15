#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../global/macros.h"

#include <QString>
#include <QWidget>
#include <QtCore>

class QObject;

namespace Ui {
class ClientConfigPage;
}

class NODISCARD_QOBJECT ClientConfigPage final : public QWidget
{
    Q_OBJECT

private:
    Ui::ClientConfigPage *const ui;

public:
    explicit ClientConfigPage(QWidget *parent);
    ~ClientConfigPage() final;

public slots:
    void slot_loadConfig();

private slots:
    void slot_onExport();
    void slot_onImport();

private:
    QString exportHotkeysToString() const;
    bool importFromString(const QString &content);
};
