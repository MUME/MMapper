// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include "commspage.h"

#include <QColorDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QVBoxLayout>

#include "../configuration/configuration.h"

CommsPage::CommsPage(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    connectSignals();
    slot_loadConfig();
}

CommsPage::~CommsPage() = default;

void CommsPage::setupUI()
{
    auto *mainLayout = new QVBoxLayout(this);

    // Talker Colors Group
    auto *talkerColorsGroup = new QGroupBox(tr("Talker Colors"), this);
    auto *talkerColorsLayout = new QFormLayout(talkerColorsGroup);

    m_talkerYouColorButton = new QPushButton(tr("Choose Color..."), this);
    m_talkerYouColorButton->setMinimumWidth(120);
    m_talkerYouColorButton->setProperty("colorType", "talker_you");
    talkerColorsLayout->addRow(tr("You (sent messages):"), m_talkerYouColorButton);

    m_talkerPlayerColorButton = new QPushButton(tr("Choose Color..."), this);
    m_talkerPlayerColorButton->setMinimumWidth(120);
    m_talkerPlayerColorButton->setProperty("colorType", "talker_player");
    talkerColorsLayout->addRow(tr("Player:"), m_talkerPlayerColorButton);

    m_talkerNpcColorButton = new QPushButton(tr("Choose Color..."), this);
    m_talkerNpcColorButton->setMinimumWidth(120);
    m_talkerNpcColorButton->setProperty("colorType", "talker_npc");
    talkerColorsLayout->addRow(tr("NPC:"), m_talkerNpcColorButton);

    m_talkerAllyColorButton = new QPushButton(tr("Choose Color..."), this);
    m_talkerAllyColorButton->setMinimumWidth(120);
    m_talkerAllyColorButton->setProperty("colorType", "talker_ally");
    talkerColorsLayout->addRow(tr("Ally:"), m_talkerAllyColorButton);

    m_talkerNeutralColorButton = new QPushButton(tr("Choose Color..."), this);
    m_talkerNeutralColorButton->setMinimumWidth(120);
    m_talkerNeutralColorButton->setProperty("colorType", "talker_neutral");
    talkerColorsLayout->addRow(tr("Neutral:"), m_talkerNeutralColorButton);

    m_talkerEnemyColorButton = new QPushButton(tr("Choose Color..."), this);
    m_talkerEnemyColorButton->setMinimumWidth(120);
    m_talkerEnemyColorButton->setProperty("colorType", "talker_enemy");
    talkerColorsLayout->addRow(tr("Enemy:"), m_talkerEnemyColorButton);

    mainLayout->addWidget(talkerColorsGroup);

    // Communication Colors Group
    auto *colorsGroup = new QGroupBox(tr("Communication Colors"), this);
    auto *colorsLayout = new QFormLayout(colorsGroup);

    // Direct communications
    m_tellColorButton = new QPushButton(tr("Choose Color..."), this);
    m_tellColorButton->setMinimumWidth(120);
    m_tellColorButton->setProperty("colorType", "tell");
    colorsLayout->addRow(tr("Tell:"), m_tellColorButton);

    m_whisperColorButton = new QPushButton(tr("Choose Color..."), this);
    m_whisperColorButton->setMinimumWidth(120);
    m_whisperColorButton->setProperty("colorType", "whisper");
    colorsLayout->addRow(tr("Whisper:"), m_whisperColorButton);

    m_groupColorButton = new QPushButton(tr("Choose Color..."), this);
    m_groupColorButton->setMinimumWidth(120);
    m_groupColorButton->setProperty("colorType", "group");
    colorsLayout->addRow(tr("Group:"), m_groupColorButton);

    m_askColorButton = new QPushButton(tr("Choose Color..."), this);
    m_askColorButton->setMinimumWidth(120);
    m_askColorButton->setProperty("colorType", "ask");
    colorsLayout->addRow(tr("Question:"), m_askColorButton);

    // Local communications
    m_sayColorButton = new QPushButton(tr("Choose Color..."), this);
    m_sayColorButton->setMinimumWidth(120);
    m_sayColorButton->setProperty("colorType", "say");
    colorsLayout->addRow(tr("Say:"), m_sayColorButton);

    m_emoteColorButton = new QPushButton(tr("Choose Color..."), this);
    m_emoteColorButton->setMinimumWidth(120);
    m_emoteColorButton->setProperty("colorType", "emote");
    colorsLayout->addRow(tr("Emote:"), m_emoteColorButton);

    m_socialColorButton = new QPushButton(tr("Choose Color..."), this);
    m_socialColorButton->setMinimumWidth(120);
    m_socialColorButton->setProperty("colorType", "social");
    colorsLayout->addRow(tr("Social:"), m_socialColorButton);

    m_yellColorButton = new QPushButton(tr("Choose Color..."), this);
    m_yellColorButton->setMinimumWidth(120);
    m_yellColorButton->setProperty("colorType", "yell");
    colorsLayout->addRow(tr("Yell:"), m_yellColorButton);

    // Global communications
    m_narrateColorButton = new QPushButton(tr("Choose Color..."), this);
    m_narrateColorButton->setMinimumWidth(120);
    m_narrateColorButton->setProperty("colorType", "narrate");
    colorsLayout->addRow(tr("Tale:"), m_narrateColorButton);

    m_singColorButton = new QPushButton(tr("Choose Color..."), this);
    m_singColorButton->setMinimumWidth(120);
    m_singColorButton->setProperty("colorType", "sing");
    colorsLayout->addRow(tr("Song:"), m_singColorButton);

    m_prayColorButton = new QPushButton(tr("Choose Color..."), this);
    m_prayColorButton->setMinimumWidth(120);
    m_prayColorButton->setProperty("colorType", "pray");
    colorsLayout->addRow(tr("Prayer:"), m_prayColorButton);

    m_shoutColorButton = new QPushButton(tr("Choose Color..."), this);
    m_shoutColorButton->setMinimumWidth(120);
    m_shoutColorButton->setProperty("colorType", "shout");
    colorsLayout->addRow(tr("Shout:"), m_shoutColorButton);

    // Background color
    m_bgColorButton = new QPushButton(tr("Choose Color..."), this);
    m_bgColorButton->setMinimumWidth(120);
    colorsLayout->addRow(tr("Background:"), m_bgColorButton);

    mainLayout->addWidget(colorsGroup);

    // Font Styling Group
    auto *fontGroup = new QGroupBox(tr("Font Styling"), this);
    auto *fontLayout = new QVBoxLayout(fontGroup);

    m_yellAllCapsCheck = new QCheckBox(tr("Display yells in ALL CAPS"), this);
    fontLayout->addWidget(m_yellAllCapsCheck);

    m_whisperItalicCheck = new QCheckBox(tr("Display whispers in italic"), this);
    fontLayout->addWidget(m_whisperItalicCheck);

    m_emoteItalicCheck = new QCheckBox(tr("Display emotes in italic"), this);
    fontLayout->addWidget(m_emoteItalicCheck);

    mainLayout->addWidget(fontGroup);

    // Display Options Group
    auto *displayGroup = new QGroupBox(tr("Display Options"), this);
    auto *displayLayout = new QVBoxLayout(displayGroup);

    m_showTimestampsCheck = new QCheckBox(tr("Show timestamps"), this);
    displayLayout->addWidget(m_showTimestampsCheck);

    mainLayout->addWidget(displayGroup);

    // Add stretch at the bottom to push everything up
    mainLayout->addStretch();

    setLayout(mainLayout);
}

void CommsPage::connectSignals()
{
    // Color buttons - all connect to the same slot
    connect(m_tellColorButton, &QPushButton::clicked, this, &CommsPage::slot_onColorClicked);
    connect(m_whisperColorButton, &QPushButton::clicked, this, &CommsPage::slot_onColorClicked);
    connect(m_groupColorButton, &QPushButton::clicked, this, &CommsPage::slot_onColorClicked);
    connect(m_askColorButton, &QPushButton::clicked, this, &CommsPage::slot_onColorClicked);
    connect(m_sayColorButton, &QPushButton::clicked, this, &CommsPage::slot_onColorClicked);
    connect(m_emoteColorButton, &QPushButton::clicked, this, &CommsPage::slot_onColorClicked);
    connect(m_socialColorButton, &QPushButton::clicked, this, &CommsPage::slot_onColorClicked);
    connect(m_yellColorButton, &QPushButton::clicked, this, &CommsPage::slot_onColorClicked);
    connect(m_narrateColorButton, &QPushButton::clicked, this, &CommsPage::slot_onColorClicked);
    connect(m_singColorButton, &QPushButton::clicked, this, &CommsPage::slot_onColorClicked);
    connect(m_prayColorButton, &QPushButton::clicked, this, &CommsPage::slot_onColorClicked);
    connect(m_shoutColorButton, &QPushButton::clicked, this, &CommsPage::slot_onColorClicked);
    connect(m_bgColorButton, &QPushButton::clicked, this, &CommsPage::slot_onBgColorClicked);

    // Talker color buttons
    connect(m_talkerYouColorButton, &QPushButton::clicked, this, &CommsPage::slot_onColorClicked);
    connect(m_talkerPlayerColorButton, &QPushButton::clicked, this, &CommsPage::slot_onColorClicked);
    connect(m_talkerNpcColorButton, &QPushButton::clicked, this, &CommsPage::slot_onColorClicked);
    connect(m_talkerAllyColorButton, &QPushButton::clicked, this, &CommsPage::slot_onColorClicked);
    connect(m_talkerNeutralColorButton, &QPushButton::clicked, this, &CommsPage::slot_onColorClicked);
    connect(m_talkerEnemyColorButton, &QPushButton::clicked, this, &CommsPage::slot_onColorClicked);

    // Font styling
    connect(m_yellAllCapsCheck, &QCheckBox::checkStateChanged, this, &CommsPage::slot_onYellAllCapsChanged);
    connect(m_whisperItalicCheck, &QCheckBox::checkStateChanged, this, &CommsPage::slot_onWhisperItalicChanged);
    connect(m_emoteItalicCheck, &QCheckBox::checkStateChanged, this, &CommsPage::slot_onEmoteItalicChanged);

    // Display options
    connect(m_showTimestampsCheck, &QCheckBox::checkStateChanged, this, &CommsPage::slot_onShowTimestampsChanged);
}

void CommsPage::slot_loadConfig()
{
    const auto &comms = getConfig().comms;

    // Load per-type colors and update button backgrounds
    updateColorButton(m_tellColorButton, comms.tellColor.get());
    updateColorButton(m_whisperColorButton, comms.whisperColor.get());
    updateColorButton(m_groupColorButton, comms.groupColor.get());
    updateColorButton(m_askColorButton, comms.askColor.get());
    updateColorButton(m_sayColorButton, comms.sayColor.get());
    updateColorButton(m_emoteColorButton, comms.emoteColor.get());
    updateColorButton(m_socialColorButton, comms.socialColor.get());
    updateColorButton(m_yellColorButton, comms.yellColor.get());
    updateColorButton(m_narrateColorButton, comms.narrateColor.get());
    updateColorButton(m_singColorButton, comms.singColor.get());
    updateColorButton(m_prayColorButton, comms.prayColor.get());
    updateColorButton(m_shoutColorButton, comms.shoutColor.get());
    updateColorButton(m_bgColorButton, comms.backgroundColor.get());

    // Load talker colors
    updateColorButton(m_talkerYouColorButton, comms.talkerYouColor.get());
    updateColorButton(m_talkerPlayerColorButton, comms.talkerPlayerColor.get());
    updateColorButton(m_talkerNpcColorButton, comms.talkerNpcColor.get());
    updateColorButton(m_talkerAllyColorButton, comms.talkerAllyColor.get());
    updateColorButton(m_talkerNeutralColorButton, comms.talkerNeutralColor.get());
    updateColorButton(m_talkerEnemyColorButton, comms.talkerEnemyColor.get());

    // Load font styling options
    m_yellAllCapsCheck->setChecked(comms.yellAllCaps.get());
    m_whisperItalicCheck->setChecked(comms.whisperItalic.get());
    m_emoteItalicCheck->setChecked(comms.emoteItalic.get());

    // Load display options
    m_showTimestampsCheck->setChecked(comms.showTimestamps.get());
}

void CommsPage::updateColorButton(QPushButton *button, const QColor &color)
{
    if (!button) {
        return;
    }

    // Set button background to the color
    button->setStyleSheet(QString("background-color: %1; color: %2;")
                              .arg(color.name())
                              .arg(color.lightnessF() > 0.5 ? "black" : "white"));
}

void CommsPage::slot_onColorClicked()
{
    auto *button = qobject_cast<QPushButton *>(sender());
    if (!button) {
        return;
    }

    const QString colorType = button->property("colorType").toString();
    auto &comms = setConfig().comms;

    QColor currentColor;
    QString dialogTitle;

    // Get current color and dialog title based on type
    if (colorType == "tell") {
        currentColor = comms.tellColor.get();
        dialogTitle = tr("Choose Tell Color");
    } else if (colorType == "whisper") {
        currentColor = comms.whisperColor.get();
        dialogTitle = tr("Choose Whisper Color");
    } else if (colorType == "group") {
        currentColor = comms.groupColor.get();
        dialogTitle = tr("Choose Group Color");
    } else if (colorType == "ask") {
        currentColor = comms.askColor.get();
        dialogTitle = tr("Choose Question Color");
    } else if (colorType == "say") {
        currentColor = comms.sayColor.get();
        dialogTitle = tr("Choose Say Color");
    } else if (colorType == "emote") {
        currentColor = comms.emoteColor.get();
        dialogTitle = tr("Choose Emote Color");
    } else if (colorType == "social") {
        currentColor = comms.socialColor.get();
        dialogTitle = tr("Choose Social Color");
    } else if (colorType == "yell") {
        currentColor = comms.yellColor.get();
        dialogTitle = tr("Choose Yell Color");
    } else if (colorType == "narrate") {
        currentColor = comms.narrateColor.get();
        dialogTitle = tr("Choose Tale Color");
    } else if (colorType == "sing") {
        currentColor = comms.singColor.get();
        dialogTitle = tr("Choose Song Color");
    } else if (colorType == "pray") {
        currentColor = comms.prayColor.get();
        dialogTitle = tr("Choose Prayer Color");
    } else if (colorType == "shout") {
        currentColor = comms.shoutColor.get();
        dialogTitle = tr("Choose Shout Color");
    } else if (colorType == "talker_you") {
        currentColor = comms.talkerYouColor.get();
        dialogTitle = tr("Choose You Color");
    } else if (colorType == "talker_player") {
        currentColor = comms.talkerPlayerColor.get();
        dialogTitle = tr("Choose Player Color");
    } else if (colorType == "talker_npc") {
        currentColor = comms.talkerNpcColor.get();
        dialogTitle = tr("Choose NPC Color");
    } else if (colorType == "talker_ally") {
        currentColor = comms.talkerAllyColor.get();
        dialogTitle = tr("Choose Ally Color");
    } else if (colorType == "talker_neutral") {
        currentColor = comms.talkerNeutralColor.get();
        dialogTitle = tr("Choose Neutral Color");
    } else if (colorType == "talker_enemy") {
        currentColor = comms.talkerEnemyColor.get();
        dialogTitle = tr("Choose Enemy Color");
    } else {
        return;
    }

    QColor newColor = QColorDialog::getColor(currentColor, this, dialogTitle);

    if (newColor.isValid() && newColor != currentColor) {
        // Set the new color based on type
        if (colorType == "tell") {
            comms.tellColor.set(newColor);
        } else if (colorType == "whisper") {
            comms.whisperColor.set(newColor);
        } else if (colorType == "group") {
            comms.groupColor.set(newColor);
        } else if (colorType == "ask") {
            comms.askColor.set(newColor);
        } else if (colorType == "say") {
            comms.sayColor.set(newColor);
        } else if (colorType == "emote") {
            comms.emoteColor.set(newColor);
        } else if (colorType == "social") {
            comms.socialColor.set(newColor);
        } else if (colorType == "yell") {
            comms.yellColor.set(newColor);
        } else if (colorType == "narrate") {
            comms.narrateColor.set(newColor);
        } else if (colorType == "sing") {
            comms.singColor.set(newColor);
        } else if (colorType == "pray") {
            comms.prayColor.set(newColor);
        } else if (colorType == "shout") {
            comms.shoutColor.set(newColor);
        } else if (colorType == "talker_you") {
            comms.talkerYouColor.set(newColor);
        } else if (colorType == "talker_player") {
            comms.talkerPlayerColor.set(newColor);
        } else if (colorType == "talker_npc") {
            comms.talkerNpcColor.set(newColor);
        } else if (colorType == "talker_ally") {
            comms.talkerAllyColor.set(newColor);
        } else if (colorType == "talker_neutral") {
            comms.talkerNeutralColor.set(newColor);
        } else if (colorType == "talker_enemy") {
            comms.talkerEnemyColor.set(newColor);
        }

        updateColorButton(button, newColor);
        emit sig_commsSettingsChanged();
    }
}

void CommsPage::slot_onBgColorClicked()
{
    const QColor currentColor = getConfig().comms.backgroundColor.get();
    QColor newColor = QColorDialog::getColor(currentColor, this, tr("Choose Background Color"));

    if (newColor.isValid() && newColor != currentColor) {
        setConfig().comms.backgroundColor.set(newColor);
        updateColorButton(m_bgColorButton, newColor);
        emit sig_commsSettingsChanged();
    }
}

void CommsPage::slot_onYellAllCapsChanged(Qt::CheckState state)
{
    setConfig().comms.yellAllCaps.set(state == Qt::Checked);
    emit sig_commsSettingsChanged();
}

void CommsPage::slot_onWhisperItalicChanged(Qt::CheckState state)
{
    setConfig().comms.whisperItalic.set(state == Qt::Checked);
    emit sig_commsSettingsChanged();
}

void CommsPage::slot_onEmoteItalicChanged(Qt::CheckState state)
{
    setConfig().comms.emoteItalic.set(state == Qt::Checked);
    emit sig_commsSettingsChanged();
}

void CommsPage::slot_onShowTimestampsChanged(Qt::CheckState state)
{
    setConfig().comms.showTimestamps.set(state == Qt::Checked);
    emit sig_commsSettingsChanged();
}
