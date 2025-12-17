#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include "../global/macros.h"

#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QWidget>

class NODISCARD_QOBJECT CommsPage final : public QWidget
{
    Q_OBJECT

public:
    explicit CommsPage(QWidget *parent);
    ~CommsPage() final;

    DELETE_CTORS_AND_ASSIGN_OPS(CommsPage);

signals:
    void sig_commsSettingsChanged();

public slots:
    void slot_loadConfig();

private slots:
    void slot_onColorClicked();
    void slot_onBgColorClicked();
    void slot_onYellAllCapsChanged(Qt::CheckState state);
    void slot_onWhisperItalicChanged(Qt::CheckState state);
    void slot_onEmoteItalicChanged(Qt::CheckState state);
    void slot_onShowTimestampsChanged(Qt::CheckState state);

private:
    void setupUI();
    void connectSignals();
    void updateColorButton(QPushButton *button, const QColor &color);

    // Color buttons (one per communication type)
    QPushButton *m_tellColorButton = nullptr;
    QPushButton *m_whisperColorButton = nullptr;
    QPushButton *m_groupColorButton = nullptr;
    QPushButton *m_askColorButton = nullptr;
    QPushButton *m_sayColorButton = nullptr;
    QPushButton *m_emoteColorButton = nullptr;
    QPushButton *m_socialColorButton = nullptr;
    QPushButton *m_yellColorButton = nullptr;
    QPushButton *m_narrateColorButton = nullptr;
    QPushButton *m_prayColorButton = nullptr;
    QPushButton *m_shoutColorButton = nullptr;
    QPushButton *m_singColorButton = nullptr;
    QPushButton *m_bgColorButton = nullptr;

    // Talker color buttons (based on GMCP talker-type)
    QPushButton *m_talkerYouColorButton = nullptr;
    QPushButton *m_talkerPlayerColorButton = nullptr;
    QPushButton *m_talkerNpcColorButton = nullptr;
    QPushButton *m_talkerAllyColorButton = nullptr;
    QPushButton *m_talkerNeutralColorButton = nullptr;
    QPushButton *m_talkerEnemyColorButton = nullptr;

    // Font styling checkboxes
    QCheckBox *m_yellAllCapsCheck = nullptr;
    QCheckBox *m_whisperItalicCheck = nullptr;
    QCheckBox *m_emoteItalicCheck = nullptr;

    // Display options
    QCheckBox *m_showTimestampsCheck = nullptr;
};
