#pragma once
/************************************************************************
**
** Authors:   Nils Schimmelmann <nschimme@gmail.com> (Jahara)
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

#include <array>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QNetworkAccessManager>

class CompareVersion final
{
public:
    explicit CompareVersion(const QString &versionStr) noexcept;
    bool operator>(const CompareVersion &other) const;
    bool operator==(const CompareVersion &other) const;
    operator QString() const;

private:
    std::array<int, 3> parts;
};

class UpdateDialog : public QDialog
{
    Q_OBJECT
public:
    explicit UpdateDialog(QWidget *parent = nullptr);

    void open() override;

private slots:
    void managerFinished(QNetworkReply *reply);
    void accepted();

private:
    QNetworkAccessManager manager;
    QLabel *text;
    QDialogButtonBox *buttonBox;
};
