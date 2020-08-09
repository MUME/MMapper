#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <memory>
#include <vector>
#include <QString>
#include <QWidget>
#include <QtCore>

#include "ui_graphicspage.h"

class AdvancedGraphicsGroupBox;
class QObject;
class XNamedColor;

namespace Ui {
class GraphicsPage;
}

class GraphicsPage : public QWidget
{
    Q_OBJECT

public:
    explicit GraphicsPage(QWidget *parent = nullptr);
    ~GraphicsPage() override;

signals:
    void sig_graphicsSettingsChanged();

public slots:
    void loadConfig();
    void antialiasingSamplesTextChanged(const QString &);
    void trilinearFilteringStateChanged(int);
    void updatedStateChanged(int);
    void drawNotMappedExitsStateChanged(int);
    void drawDoorNamesStateChanged(int);
    void drawUpperLayersTexturedStateChanged(int);

private:
    void changeColorClicked(XNamedColor &color, QPushButton *pushButton);
    void graphicsSettingsChanged() { emit sig_graphicsSettingsChanged(); }
    Ui::GraphicsPage *const ui;
    std::unique_ptr<AdvancedGraphicsGroupBox> m_advanced;
};
