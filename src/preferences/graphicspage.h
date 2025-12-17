#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/macros.h"
#include "ui_graphicspage.h"

#include <memory>
#include <vector>

#include <QString>
#include <QWidget>
#include <QtCore>

class AdvancedGraphicsGroupBox;
class QObject;
class XNamedColor;

namespace Ui {
class GraphicsPage;
}

class NODISCARD_QOBJECT GraphicsPage final : public QWidget
{
    Q_OBJECT

public:
    explicit GraphicsPage(QWidget *parent);
    ~GraphicsPage() final;

private:
    void changeColorClicked(XNamedColor &color, QPushButton *pushButton);
    void graphicsSettingsChanged() { emit sig_graphicsSettingsChanged(); }
    Ui::GraphicsPage *const ui;
    std::unique_ptr<AdvancedGraphicsGroupBox> m_advanced;

signals:
    void sig_graphicsSettingsChanged();
    void sig_textureSettingsChanged();

public slots:
    void slot_loadConfig();
    void slot_antialiasingSamplesTextChanged(const QString &);
    void slot_trilinearFilteringStateChanged(int);
    void slot_drawNeedsUpdateStateChanged(int);
    void slot_drawNotMappedExitsStateChanged(int);
    void slot_drawDoorNamesStateChanged(int);
    void slot_drawUpperLayersTexturedStateChanged(int);
    void slot_textureSetChanged(int index);
    void slot_enableSeasonalTexturesStateChanged(int state);
    // this slot just calls the signal... not useful
    void slot_graphicsSettingsChanged() { graphicsSettingsChanged(); }
};
