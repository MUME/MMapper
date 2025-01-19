#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Jan 'Kovis' Struhar <kovis@sourceforge.net> (Kovis)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/macros.h"

#include <memory>

#include <QDialog>

namespace Ui {
class AnsiColorDialog;
}

class NODISCARD_QOBJECT AnsiColorDialog final : public QDialog
{
    Q_OBJECT

public:
    NODISCARD static QString getColor(const QString &ansiString, QWidget *parent);

private:
    QString m_ansiString;
    const std::unique_ptr<Ui::AnsiColorDialog> m_ui;

public:
    explicit AnsiColorDialog(const QString &ansiString, QWidget *parent);
    explicit AnsiColorDialog(QWidget *parent);
    ~AnsiColorDialog() final;

public:
    NODISCARD QString getAnsiString() const { return m_ansiString; }

private:
    void ansiComboChange();

public slots:
    void slot_ansiComboChange();
    void slot_updateColors();
    void slot_generateNewAnsiColor();
};
