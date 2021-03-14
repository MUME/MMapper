#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Jan 'Kovis' Struhar <kovis@sourceforge.net> (Kovis)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <QDialog>

#include "../global/macros.h"

namespace Ui {
class AnsiColorDialog;
}

class AnsiColorDialog final : public QDialog
{
    Q_OBJECT

public:
    explicit AnsiColorDialog(const QString &ansiString, QWidget *parent = nullptr);
    explicit AnsiColorDialog(QWidget *parent = nullptr);
    ~AnsiColorDialog() final;

    NODISCARD QString getAnsiString() const { return ansiString; }
    NODISCARD static QString getColor(const QString &ansiString, QWidget *parent = nullptr);

public slots:
    void slot_ansiComboChange();
    void slot_updateColors();
    void slot_generateNewAnsiColor();

signals:
    void sig_newAnsiString(const QString &);

private:
    QString ansiString;
    Ui::AnsiColorDialog *const ui;
};
