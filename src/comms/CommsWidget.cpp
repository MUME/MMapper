// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include "CommsWidget.h"

#include <algorithm>
#include <QTextCursor>
#include <QTextCharFormat>
#include <QScrollBar>
#include <QLabel>
#include <QGroupBox>
#include <QRegularExpression>
#include <QFileDialog>
#include <QFile>
#include <QDateTime>
#include <QDir>
#include <QMessageBox>

#include "../configuration/configuration.h"
#include "../logger/autologger.h"
#include "../global/TextUtils.h"

CommsWidget::CommsWidget(CommsManager &commsManager, AutoLogger *autoLogger, QWidget *parent)
    : QWidget(parent)
    , m_commsManager{commsManager}
    , m_autoLogger{autoLogger}
{
    // Initialize all filter states to enabled (not muted)
    m_filterStates[CommType::TELL] = true;
    m_filterStates[CommType::WHISPER] = true;
    m_filterStates[CommType::GROUP] = true;
    m_filterStates[CommType::ASK] = true;
    m_filterStates[CommType::EMOTE] = true;
    m_filterStates[CommType::SOCIAL] = true;
    m_filterStates[CommType::SAY] = true;
    m_filterStates[CommType::YELL] = true;
    m_filterStates[CommType::NARRATE] = true;
    m_filterStates[CommType::SING] = true;
    m_filterStates[CommType::PRAY] = true;

    setupUI();
    connectSignals();
    slot_loadSettings();
}

CommsWidget::~CommsWidget() = default;

void CommsWidget::setupUI()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(2, 2, 2, 2);
    mainLayout->setSpacing(4);

    // Top row: Filter buttons and C&M toggle
    auto *topLayout = new QHBoxLayout();
    topLayout->setSpacing(4);

    // Helper to create a filter button
    auto createFilterButton = [this](const QString &fullLabel, const QString &shortLabel, CommType type) {
        auto *btn = new QPushButton(fullLabel);
        btn->setToolTip(fullLabel);
        btn->setProperty("shortLabel", shortLabel);
        btn->setProperty("fullLabel", fullLabel);
        btn->setCheckable(true);
        btn->setChecked(true); // Initially not muted
        btn->setMaximumHeight(24);
        btn->setMinimumWidth(40);
        m_filterButtons[type] = btn;

        connect(btn, &QPushButton::toggled, this, [this, type](bool checked) {
            slot_onFilterToggled(type, checked);
        });

        return btn;
    };

    // Direct group
    auto *directGroup = new QWidget();
    auto *directLayout = new QVBoxLayout(directGroup);
    directLayout->setContentsMargins(0, 0, 0, 0);
    directLayout->setSpacing(2);

    auto *directLabel = new QLabel("<b>Direct</b>");
    directLabel->setAlignment(Qt::AlignCenter);
    directLayout->addWidget(directLabel);

    auto *directButtonsLayout = new QHBoxLayout();
    directButtonsLayout->setSpacing(2);
    directButtonsLayout->addWidget(createFilterButton("Tells", "Te", CommType::TELL));
    directButtonsLayout->addWidget(createFilterButton("Qtions", "Qt", CommType::ASK));
    directButtonsLayout->addWidget(createFilterButton("Whispers", "Wh", CommType::WHISPER));
    directButtonsLayout->addWidget(createFilterButton("Group", "Gr", CommType::GROUP));
    directLayout->addLayout(directButtonsLayout);

    // Local group
    auto *localGroup = new QWidget();
    auto *localLayout = new QVBoxLayout(localGroup);
    localLayout->setContentsMargins(0, 0, 0, 0);
    localLayout->setSpacing(2);

    auto *localLabel = new QLabel("<b>Local</b>");
    localLabel->setAlignment(Qt::AlignCenter);
    localLayout->addWidget(localLabel);

    auto *localButtonsLayout = new QHBoxLayout();
    localButtonsLayout->setSpacing(2);
    localButtonsLayout->addWidget(createFilterButton("Emotes", "Em", CommType::EMOTE));
    localButtonsLayout->addWidget(createFilterButton("Socials", "So", CommType::SOCIAL));
    localButtonsLayout->addWidget(createFilterButton("Says", "Sa", CommType::SAY));
    localButtonsLayout->addWidget(createFilterButton("Yells", "Ye", CommType::YELL));
    localLayout->addLayout(localButtonsLayout);

    // Global group
    auto *globalGroup = new QWidget();
    auto *globalLayout = new QVBoxLayout(globalGroup);
    globalLayout->setContentsMargins(0, 0, 0, 0);
    globalLayout->setSpacing(2);

    auto *globalLabel = new QLabel("<b>Global</b>");
    globalLabel->setAlignment(Qt::AlignCenter);
    globalLayout->addWidget(globalLabel);

    auto *globalButtonsLayout = new QHBoxLayout();
    globalButtonsLayout->setSpacing(2);
    globalButtonsLayout->addWidget(createFilterButton("Tales", "Ta", CommType::NARRATE));
    globalButtonsLayout->addWidget(createFilterButton("Songs", "Sn", CommType::SING));
    globalButtonsLayout->addWidget(createFilterButton("Prayers", "Pr", CommType::PRAY));
    globalLayout->addLayout(globalButtonsLayout);

    // Add groups to top layout
    topLayout->addWidget(directGroup);
    topLayout->addWidget(localGroup);
    topLayout->addWidget(globalGroup);
    topLayout->addStretch();

    // C&M toggle button (Characters and Mobs)
    m_charMobToggle = new QToolButton();
    m_charMobToggle->setText("C&M");
    m_charMobToggle->setToolTip("Toggle between Characters, Mobs, or All");
    m_charMobToggle->setMaximumSize(32, 24);
    connect(m_charMobToggle, &QToolButton::clicked, this, &CommsWidget::slot_onCharMobToggle);
    updateCharMobButtonAppearance();
    topLayout->addWidget(m_charMobToggle);

    mainLayout->addLayout(topLayout);

    // Main text display
    m_textDisplay = new QTextEdit();
    m_textDisplay->setReadOnly(true);
    m_textDisplay->setLineWrapMode(QTextEdit::WidgetWidth);
    m_textDisplay->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    mainLayout->addWidget(m_textDisplay);

    setLayout(mainLayout);
}

void CommsWidget::connectSignals()
{
    // Connect to CommsManager signals
    connect(&m_commsManager,
            &CommsManager::sig_newMessage,
            this,
            &CommsWidget::slot_onNewMessage);
}

void CommsWidget::slot_loadSettings()
{
    const auto &comms = getConfig().comms;

    // Apply background color
    QPalette palette = m_textDisplay->palette();
    palette.setColor(QPalette::Base, comms.backgroundColor.get());
    m_textDisplay->setPalette(palette);

    // Refresh display to apply any color/style changes
    rebuildDisplay();
}

void CommsWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateButtonLabels();
}

void CommsWidget::updateButtonLabels()
{
    const int availableWidth = width();
    const bool useShortLabels = availableWidth < 600;  // Threshold for switching to short labels

    for (auto it = m_filterButtons.constBegin(); it != m_filterButtons.constEnd(); ++it) {
        auto *btn = it.value();
        const QString fullLabel = btn->property("fullLabel").toString();
        const QString shortLabel = btn->property("shortLabel").toString();
        btn->setText(useShortLabels ? shortLabel : fullLabel);
    }
}

void CommsWidget::slot_onFilterToggled(CommType type, bool enabled)
{
    m_filterStates[type] = enabled;
    updateFilterButtonAppearance(m_filterButtons[type], enabled);
    rebuildDisplay();
}

void CommsWidget::slot_onCharMobToggle()
{
    // Cycle through: C&M -> C -> M -> C&M
    switch (m_charMobFilter) {
    case CharMobFilterEnum::BOTH:
        m_charMobFilter = CharMobFilterEnum::CHAR_ONLY;
        break;
    case CharMobFilterEnum::CHAR_ONLY:
        m_charMobFilter = CharMobFilterEnum::MOB_ONLY;
        break;
    case CharMobFilterEnum::MOB_ONLY:
        m_charMobFilter = CharMobFilterEnum::BOTH;
        break;
    }

    updateCharMobButtonAppearance();
    rebuildDisplay();
}

void CommsWidget::rebuildDisplay()
{
    m_textDisplay->clear();
    for (const auto &cached : m_messageCache) {
        if (!isMessageFiltered(cached.msg)) {
            appendFormattedMessage(cached.msg);
        }
    }
}

void CommsWidget::slot_onNewMessage(const CommMessage &msg)
{
    // Cache the message (always with timestamp)
    m_messageCache.append({msg});

    // Limit cache size
    while (m_messageCache.size() > MAX_MESSAGES) {
        m_messageCache.removeFirst();
    }

    // Only display if not filtered
    if (!isMessageFiltered(msg)) {
        appendFormattedMessage(msg);
    }
}

void CommsWidget::appendFormattedMessage(const CommMessage &msg)
{
    QTextCursor cursor(m_textDisplay->document());
    cursor.movePosition(QTextCursor::End);

    // Add timestamp if enabled
    if (getConfig().comms.showTimestamps.get()) {
        QTextCharFormat timestampFormat;
        timestampFormat.setForeground(QColor(128, 128, 128)); // Gray
        cursor.setCharFormat(timestampFormat);
        cursor.insertText(QString("[%1] ").arg(msg.timestamp));
    }

    // Strip ANSI codes first
    QString originalSender = stripAnsiCodes(msg.sender);
    QString message = stripAnsiCodes(msg.message);

    // Clean sender name (remove articles, capitalize)
    QString cleanedSender = cleanSenderName(originalSender);

    // Check if message already contains formatting (double caption issue)
    // Check against ORIGINAL sender name before cleaning
    if (message.startsWith(originalSender, Qt::CaseInsensitive)) {
        // Message already contains the full formatted text
        // We need to replace the original sender with cleaned sender and use that
        QTextCharFormat nameFormat;
        nameFormat.setForeground(getColorForTalker(msg.talkerType));  // Use talker color for names
        nameFormat.setFontWeight(QFont::Bold);

        QTextCharFormat textFormat;
        textFormat.setForeground(getColorForType(msg.type));
        textFormat.setFontWeight(QFont::Normal);

        // Apply italic for whispers/emotes/socials if configured
        if ((msg.type == CommType::WHISPER && getConfig().comms.whisperItalic.get())
            || ((msg.type == CommType::EMOTE || msg.type == CommType::SOCIAL) && getConfig().comms.emoteItalic.get())) {
            textFormat.setFontItalic(true);
            nameFormat.setFontItalic(true);  // Name also italic for emotes/socials
        }

        // Insert cleaned sender name in bold
        cursor.setCharFormat(nameFormat);
        cursor.insertText(cleanedSender);

        // Insert rest of message (skip original sender part)
        cursor.setCharFormat(textFormat);
        cursor.insertText(message.mid(originalSender.length()) + "\n");
    } else {
        // Format the message ourselves
        formatAndInsertMessage(cursor, msg, cleanedSender, message);
    }

    // Auto-scroll to bottom
    m_textDisplay->verticalScrollBar()->setValue(m_textDisplay->verticalScrollBar()->maximum());
}

void CommsWidget::formatAndInsertMessage(QTextCursor &cursor, const CommMessage &msg, const QString &sender, const QString &message)
{
    // Apply transformations
    QString finalMessage = message;
    if (msg.type == CommType::YELL && getConfig().comms.yellAllCaps.get()) {
        // Only uppercase the message text, not qualifier prefix like "[faintly from below]"
        static const QRegularExpression qualifierPrefix(R"(^(\[.+?\] )(.*)$)");
        auto match = qualifierPrefix.match(finalMessage);
        if (match.hasMatch()) {
            // Keep prefix lowercase, uppercase the message
            finalMessage = match.captured(1) + match.captured(2).toUpper();
        } else {
            // No prefix, uppercase everything
            finalMessage = finalMessage.toUpper();
        }
    }

    // Format: [Bold Name] verb: 'message'
    QTextCharFormat nameFormat;
    nameFormat.setForeground(getColorForTalker(msg.talkerType));  // Use talker color for name
    nameFormat.setFontWeight(QFont::Bold);  // Only name is bold

    QTextCharFormat textFormat;
    textFormat.setForeground(getColorForType(msg.type));
    textFormat.setFontWeight(QFont::Normal);  // Rest is normal

    // Apply italic for whispers/emotes if configured
    if ((msg.type == CommType::WHISPER && getConfig().comms.whisperItalic.get())
        || ((msg.type == CommType::EMOTE || msg.type == CommType::SOCIAL) && getConfig().comms.emoteItalic.get())) {
        textFormat.setFontItalic(true);
    }

    // Simplified format: "Name: 'message'" for all types
    if (msg.type == CommType::PRAY) {
        // Special case for prayer (no sender from others)
        cursor.setCharFormat(nameFormat);
        cursor.insertText("You");
        cursor.setCharFormat(textFormat);
        cursor.insertText(QString(": %1\n").arg(finalMessage));
    } else {
        // Standard format
        cursor.setCharFormat(nameFormat);
        cursor.insertText(sender);
        cursor.setCharFormat(textFormat);

        // For emotes and socials, no colon (just "Name message")
        if (msg.type == CommType::EMOTE || msg.type == CommType::SOCIAL) {
            cursor.insertText(QString(" %1\n").arg(finalMessage));
        } else {
            // All other types: "Name: 'message'"
            cursor.insertText(QString(": %1\n").arg(finalMessage));
        }
    }
}

QString CommsWidget::stripAnsiCodes(const QString &text)
{
    // ANSI escape sequence pattern: \x1b[...m or similar
    static const QRegularExpression ansiPattern(R"(\x1b\[[0-9;]*m)");
    QString cleaned = text;
    cleaned.remove(ansiPattern);

    // Also handle the ∂[ format seen in the screenshot
    static const QRegularExpression altAnsiPattern(R"(∂\[[0-9;]*m?)");
    cleaned.remove(altAnsiPattern);

    return cleaned;
}

QString CommsWidget::cleanSenderName(const QString &sender)
{
    QString cleaned = sender;

    // Remove leading articles (case insensitive)
    static const QRegularExpression articlePattern(R"(^(an?)\s+)", QRegularExpression::CaseInsensitiveOption);
    cleaned.remove(articlePattern);

    // Capitalize first letter
    if (!cleaned.isEmpty() && cleaned[0].isLetter()) {
        cleaned[0] = cleaned[0].toUpper();
    }

    return cleaned;
}

QColor CommsWidget::getColorForType(CommType type)
{
    const auto &comms = getConfig().comms;

    switch (type) {
    case CommType::TELL:
        return comms.tellColor.get();
    case CommType::WHISPER:
        return comms.whisperColor.get();
    case CommType::GROUP:
        return comms.groupColor.get();
    case CommType::ASK:
        return comms.askColor.get();
    case CommType::SAY:
        return comms.sayColor.get();
    case CommType::EMOTE:
        return comms.emoteColor.get();
    case CommType::SOCIAL:
        return comms.socialColor.get();
    case CommType::YELL:
        return comms.yellColor.get();
    case CommType::NARRATE:
        return comms.narrateColor.get();
    case CommType::PRAY:
        return comms.prayColor.get();
    case CommType::SHOUT:
        return comms.shoutColor.get();
    case CommType::SING:
        return comms.singColor.get();
    default:
        return Qt::white;
    }
}

QColor CommsWidget::getColorForTalker(TalkerType talkerType)
{
    const auto &comms = getConfig().comms;

    switch (talkerType) {
    case TalkerType::YOU:
        return comms.talkerYouColor.get();
    case TalkerType::PLAYER:
        return comms.talkerPlayerColor.get();
    case TalkerType::NPC:
        return comms.talkerNpcColor.get();
    case TalkerType::ALLY:
        return comms.talkerAllyColor.get();
    case TalkerType::NEUTRAL:
        return comms.talkerNeutralColor.get();
    case TalkerType::ENEMY:
        return comms.talkerEnemyColor.get();
    default:
        return Qt::white;
    }
}

bool CommsWidget::isMessageFiltered(const CommMessage &msg) const
{
    // Check type filter
    if (!m_filterStates.value(msg.type, true)) {
        return true; // Filtered out (muted)
    }

    // Check character/mob filter
    switch (m_charMobFilter) {
    case CharMobFilterEnum::CHAR_ONLY:
        if (msg.talkerType == TalkerType::NPC) {
            return true; // Filter out NPCs
        }
        break;
    case CharMobFilterEnum::MOB_ONLY:
        if (msg.talkerType != TalkerType::NPC) {
            return true; // Filter out characters
        }
        break;
    case CharMobFilterEnum::BOTH:
        // Show all
        break;
    }

    return false;
}

void CommsWidget::updateFilterButtonAppearance(QPushButton *button, bool enabled)
{
    if (enabled) {
        // Normal appearance (not muted)
        button->setStyleSheet("");
    } else {
        // Red appearance (muted)
        button->setStyleSheet("QPushButton { background-color: #8B0000; color: white; }");
    }
}

void CommsWidget::updateCharMobButtonAppearance()
{
    switch (m_charMobFilter) {
    case CharMobFilterEnum::BOTH:
        m_charMobToggle->setText("C&M");
        m_charMobToggle->setToolTip("Showing both Characters and Mobs");
        break;
    case CharMobFilterEnum::CHAR_ONLY:
        m_charMobToggle->setText("C");
        m_charMobToggle->setToolTip("Showing Characters only");
        break;
    case CharMobFilterEnum::MOB_ONLY:
        m_charMobToggle->setText("M");
        m_charMobToggle->setToolTip("Showing Mobs only");
        break;
    }
}

void CommsWidget::slot_saveLog()
{
    struct NODISCARD Result
    {
        QStringList filenames;
        bool isHtml = false;
    };
    const auto getFileNames = [this]() -> Result {
        auto save = std::make_unique<QFileDialog>(this, "Choose communications log file name ...");
        save->setFileMode(QFileDialog::AnyFile);
        save->setDirectory(QDir::current());
        save->setNameFilters(QStringList() << "Text log (*.log *.txt)"
                                           << "HTML log (*.htm *.html)");
        save->setDefaultSuffix("txt");
        save->setAcceptMode(QFileDialog::AcceptSave);

        if (save->exec() == QDialog::Accepted) {
            const QString nameFilter = save->selectedNameFilter().toLower();
            const bool isHtml = nameFilter.endsWith(".htm") || nameFilter.endsWith(".html");
            return Result{save->selectedFiles(), isHtml};
        }

        return Result{};
    };

    const auto result = getFileNames();
    const auto &fileNames = result.filenames;

    if (fileNames.isEmpty()) {
        return;
    }

    QFile document(fileNames[0]);
    if (!document.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::warning(this,
                           tr("Save Error"),
                           tr("Error occurred while opening %1").arg(document.fileName()));
        return;
    }

    const auto getDocStringUtf8 = [](const QTextDocument *const pDoc,
                                     const bool isHtml) -> QByteArray {
        const QString string = isHtml ? pDoc->toHtml() : pDoc->toPlainText();
        return string.toUtf8();
    };
    document.write(getDocStringUtf8(m_textDisplay->document(), result.isHtml));
    document.close();
}

void CommsWidget::slot_saveLogOnExit()
{
    const auto &comms = getConfig().comms;
    if (!comms.saveLogOnExit.get()) {
        return;
    }

    // Use the same directory as AutoLogger
    const auto &autoLogConfig = getConfig().autoLog;
    QString logDir = autoLogConfig.autoLogDirectory;
    if (logDir.isEmpty()) {
        logDir = QDir::current().path();
    }

    // Create directory if it doesn't exist
    QDir dir(logDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    // Generate filename matching AutoLogger format: Comms_Log_{date}_{filenum}_{runId}.txt
    QString fileName;
    if (m_autoLogger) {
        // Note: getCurrentFileNumber() returns the NEXT file number, so we subtract 1
        // to get the current file number that matches the active MMapper log
        const int currentFileNum = std::max(0, m_autoLogger->getCurrentFileNumber() - 1);
        fileName = QString("Comms_Log_%1_%2_%3.txt")
                       .arg(QDate::currentDate().toString("yyyy_MM_dd"))
                       .arg(QString::number(currentFileNum))
                       .arg(mmqt::toQStringUtf8(m_autoLogger->getRunId()));
    } else {
        // Fallback if AutoLogger is not available
        const QString timestamp = QDateTime::currentDateTime().toString("yyyy_MM_dd_HH_mm_ss");
        fileName = QString("Comms_Log_%1.txt").arg(timestamp);
    }

    const QString fullPath = dir.filePath(fileName);
    QFile document(fullPath);
    if (!document.open(QFile::WriteOnly | QFile::Text)) {
        qWarning() << "Failed to save communications log to" << fullPath;
        return;
    }

    const QString plainText = m_textDisplay->document()->toPlainText();
    document.write(plainText.toUtf8());
    document.close();
    qInfo() << "Communications log saved to" << fullPath;
}
