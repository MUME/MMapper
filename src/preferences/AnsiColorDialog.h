#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Jan 'Kovis' Struhar <kovis@sourceforge.net> (Kovis)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <QDialog>

namespace Ui {
class AnsiColorDialog;
}

class AnsiColorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AnsiColorDialog(const QString &ansiString, QWidget *parent = nullptr);
    explicit AnsiColorDialog(QWidget *parent = nullptr);
    ~AnsiColorDialog();

    QString getAnsiString() const { return ansiString; }

    static QString getColor(const QString &ansiString, QWidget *parent = nullptr);

public slots:
    void ansiComboChange();
    void updateColors();
    void generateNewAnsiColor();

signals:
    void newAnsiString(const QString &);

private:
    QString ansiString{};
    Ui::AnsiColorDialog *ui;
};
