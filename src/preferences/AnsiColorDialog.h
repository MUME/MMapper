#pragma once
/************************************************************************
**
** Authors:   Jan 'Kovis' Struhar <kovis@sourceforge.net> (Kovis)
**            Marek Krejza <krejza@gmail.com> (Caligor)
**            Nils Schimmelmann <nschimme@gmail.com> (Jahara)
**
** This file is part of the MMapper project.
** Maintained by Nils Schimmelmann <nschimme@gmail.com>
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the:
** Free Software Foundation, Inc.
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
************************************************************************/

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
