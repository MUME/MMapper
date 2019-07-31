#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <QString>
#include <QWidget>
#include <QtCore>

#include "ui_graphicspage.h"

class QObject;

namespace Ui {
class GraphicsPage;
}

class GraphicsPage : public QWidget
{
    Q_OBJECT

public:
    explicit GraphicsPage(QWidget *parent = nullptr);

signals:

public slots:
    void antialiasingSamplesTextChanged(const QString &);
    void trilinearFilteringStateChanged(int);
    void softwareOpenGLStateChanged(int);

    void updatedStateChanged(int);
    void drawNotMappedExitsStateChanged(int);
    void drawNoMatchExitsStateChanged(int);
    void drawDoorNamesStateChanged(int);
    void drawUpperLayersTexturedStateChanged(int);

private:
    void changeColorClicked(QColor &color, QPushButton *pushButton);
    Ui::GraphicsPage *const ui;
};
