#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Kalev Lember <kalev@smartlink.ee> (Kalev)

#include "ui_aboutdialog.h"

#include <QDialog>
#include <QString>
#include <QtCore>

class QObject;
class QTextBrowser;
class QWidget;

class AboutDialog : public QDialog, private Ui::AboutDialog
{
    Q_OBJECT

public:
    explicit AboutDialog(QWidget *parent);

private:
    void setFixedFont(QTextBrowser *browser);
};
