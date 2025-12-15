// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "inputwidget.h"

#include "../configuration/configuration.h"
#include "../configuration/HotkeyManager.h"
#include "../global/Color.h"
#include "../mpi/remoteeditwidget.h"

#include <QFont>
#include <QMessageLogContext>
#include <QRegularExpression>
#include <QSize>
#include <QString>
#include <QTextStream>
#include <QtGui>
#include <QtWidgets>

static constexpr const int MIN_WORD_LENGTH = 3;

static const QRegularExpression g_whitespaceRx(R"(\s+)");

// Helper functions for key name mapping
static QString getNumpadKeyName(int key)
{
    switch (key) {
    case Qt::Key_0:
        return "NUMPAD0";
    case Qt::Key_1:
        return "NUMPAD1";
    case Qt::Key_2:
        return "NUMPAD2";
    case Qt::Key_3:
        return "NUMPAD3";
    case Qt::Key_4:
        return "NUMPAD4";
    case Qt::Key_5:
        return "NUMPAD5";
    case Qt::Key_6:
        return "NUMPAD6";
    case Qt::Key_7:
        return "NUMPAD7";
    case Qt::Key_8:
        return "NUMPAD8";
    case Qt::Key_9:
        return "NUMPAD9";
    case Qt::Key_Slash:
        return "NUMPAD_SLASH";
    case Qt::Key_Asterisk:
        return "NUMPAD_ASTERISK";
    case Qt::Key_Minus:
        return "NUMPAD_MINUS";
    case Qt::Key_Plus:
        return "NUMPAD_PLUS";
    case Qt::Key_Period:
        return "NUMPAD_PERIOD";
    default:
        return QString();
    }
}

static QString getNavigationKeyName(int key)
{
    switch (key) {
    case Qt::Key_Home:
        return "HOME";
    case Qt::Key_End:
        return "END";
    case Qt::Key_Insert:
    case Qt::Key_Help: // macOS maps Insert to Help
        return "INSERT";
    default:
        return QString();
    }
}

static QString getMiscKeyName(int key)
{
    switch (key) {
    case Qt::Key_QuoteLeft:
        return "ACCENT";
    case Qt::Key_1:
        return "1";
    case Qt::Key_2:
        return "2";
    case Qt::Key_3:
        return "3";
    case Qt::Key_4:
        return "4";
    case Qt::Key_5:
        return "5";
    case Qt::Key_6:
        return "6";
    case Qt::Key_7:
        return "7";
    case Qt::Key_8:
        return "8";
    case Qt::Key_9:
        return "9";
    case Qt::Key_0:
        return "0";
    case Qt::Key_Minus:
        return "HYPHEN";
    case Qt::Key_Equal:
        return "EQUAL";
    default:
        return QString();
    }
}

static KeyClassification classifyKey(int key, Qt::KeyboardModifiers mods)
{
    KeyClassification result;
    result.realModifiers = mods & ~Qt::KeypadModifier;

    // Function keys F1-F12 (always handled)
    if (key >= Qt::Key_F1 && key <= Qt::Key_F12) {
        result.type = KeyType::FunctionKey;
        result.keyName = QString("F%1").arg(key - Qt::Key_F1 + 1);
        result.shouldHandle = true;
        return result;
    }

    // Numpad keys (only with KeypadModifier)
    if (mods & Qt::KeypadModifier) {
        QString name = getNumpadKeyName(key);
        if (!name.isEmpty()) {
            result.type = KeyType::NumpadKey;
            result.keyName = name;
            result.shouldHandle = true;
            return result;
        }
    }

    // Navigation keys (HOME, END, INSERT - from any source)
    {
        QString name = getNavigationKeyName(key);
        if (!name.isEmpty()) {
            result.type = KeyType::NavigationKey;
            result.keyName = name;
            result.shouldHandle = true;
            return result;
        }
    }

    // Arrow keys (UP, DOWN, LEFT, RIGHT)
    if (key == Qt::Key_Up || key == Qt::Key_Down || key == Qt::Key_Left || key == Qt::Key_Right) {
        result.type = KeyType::ArrowKey;
        switch (key) {
        case Qt::Key_Up:
            result.keyName = "UP";
            break;
        case Qt::Key_Down:
            result.keyName = "DOWN";
            break;
        case Qt::Key_Left:
            result.keyName = "LEFT";
            break;
        case Qt::Key_Right:
            result.keyName = "RIGHT";
            break;
        }
        result.shouldHandle = true;
        return result;
    }

    // Misc keys (only when NOT from numpad)
    if (!(mods & Qt::KeypadModifier)) {
        QString name = getMiscKeyName(key);
        if (!name.isEmpty()) {
            result.type = KeyType::MiscKey;
            result.keyName = name;
            result.shouldHandle = true;
            return result;
        }
    }

    // Terminal shortcuts (Ctrl+U, Ctrl+W, Ctrl+H or Cmd+U, Cmd+W, Cmd+H)
    if ((key == Qt::Key_U || key == Qt::Key_W || key == Qt::Key_H)
        && (result.realModifiers == Qt::ControlModifier
            || result.realModifiers == Qt::MetaModifier)) {
        result.type = KeyType::TerminalShortcut;
        result.shouldHandle = true;
        return result;
    }

    // Basic keys (Tab, Enter - only without modifiers)
    if ((key == Qt::Key_Tab || key == Qt::Key_Return || key == Qt::Key_Enter)
        && result.realModifiers == Qt::NoModifier) {
        result.type = KeyType::BasicKey;
        result.shouldHandle = true;
        return result;
    }

    // Page keys (PageUp, PageDown - for scrolling display)
    if (key == Qt::Key_PageUp || key == Qt::Key_PageDown) {
        result.type = KeyType::PageKey;
        result.keyName = (key == Qt::Key_PageUp) ? "PAGEUP" : "PAGEDOWN";
        result.shouldHandle = true;
        return result;
    }

    return result;
}

InputWidgetOutputs::~InputWidgetOutputs() = default;

InputWidget::InputWidget(QWidget *const parent, InputWidgetOutputs &outputs)
    : QPlainTextEdit(parent)
    , m_outputs{outputs}
{
    // Size Policy
    setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));

    // Terminal Font
    QFont font;
    font.fromString(getConfig().integratedClient.font);
    setFont(font);

    // Minimum Size
    QFontMetrics fm(font);
    setMinimumSize(
        QSize(fm.averageCharWidth(),
              fm.lineSpacing()
                  + (static_cast<int>(document()->documentMargin()) + QFrame::frameWidth()) * 2
                  + contentsMargins().top() + contentsMargins().bottom()));
    setSizeIncrement(fm.averageCharWidth(), fm.lineSpacing());

    // Line Wrapping
    setLineWrapMode(QPlainTextEdit::NoWrap);

    // Remember native palletes
    m_paletteManager.init(*this, std::nullopt, Qt::lightGray);
}

QSize InputWidget::sizeHint() const
{
    return minimumSize();
}

InputWidget::~InputWidget() = default;

void InputWidget::keyPressEvent(QKeyEvent *const event)
{
    // Check if this key was already handled in ShortcutOverride
    if (m_handledInShortcutOverride) {
        qDebug() << "[InputWidget::keyPressEvent] Skipping - already handled in ShortcutOverride";
        m_handledInShortcutOverride = false; // Reset for next key
        event->accept();
        return;
    }

    const auto key = event->key();
    const auto mods = event->modifiers();

    // Handle tabbing state (unchanged)
    if (m_tabbing) {
        if (key != Qt::Key_Tab) {
            m_tabbing = false;
        }

        // If Backspace or Escape is pressed, reject the completion
        QTextCursor current = textCursor();
        if (key == Qt::Key_Backspace || key == Qt::Key_Escape) {
            current.removeSelectedText();
            event->accept();
            return;
        }

        // For any other key press, accept the completion
        if (key != Qt::Key_Tab) {
            current.movePosition(QTextCursor::Right,
                                 QTextCursor::MoveAnchor,
                                 static_cast<int>(current.selectedText().length()));
            setTextCursor(current);
        }
    }

    // Debug logging
    qDebug() << "[InputWidget::keyPressEvent] Key:" << key << "Modifiers:" << mods;

    // Classify the key ONCE
    auto classification = classifyKey(key, mods);

    if (classification.shouldHandle) {
        switch (classification.type) {
        case KeyType::FunctionKey:
            functionKeyPressed(classification.keyName, classification.realModifiers);
            event->accept();
            return;

        case KeyType::NumpadKey:
            if (numpadKeyPressed(key, classification.realModifiers)) {
                event->accept();
                return;
            }
            break;

        case KeyType::NavigationKey:
            if (navigationKeyPressed(key, classification.realModifiers)) {
                event->accept();
                return;
            }
            break;

        case KeyType::ArrowKey:
            if (arrowKeyPressed(key, classification.realModifiers)) {
                event->accept();
                return;
            }
            break;

        case KeyType::MiscKey:
            if (miscKeyPressed(key, classification.realModifiers)) {
                event->accept();
                return;
            }
            break;

        case KeyType::TerminalShortcut:
            if (handleTerminalShortcut(key)) {
                event->accept();
                return;
            }
            break;

        case KeyType::BasicKey:
            if (handleBasicKey(key)) {
                event->accept();
                return;
            }
            break;

        case KeyType::PageKey:
            if (handlePageKey(key, classification.realModifiers)) {
                event->accept();
                return;
            }
            break;

        case KeyType::Other:
            break;
        }
    }

    // All other input
    base::keyPressEvent(event);
}

void InputWidget::functionKeyPressed(const QString &keyName, Qt::KeyboardModifiers modifiers)
{
    QString fullKeyString = buildHotkeyString(keyName, modifiers);

    qDebug() << "[InputWidget::functionKeyPressed] Function key pressed:" << fullKeyString;

    // Check if there's a configured hotkey for this key combination
    const QString command = getConfig().hotkeyManager.getCommand(fullKeyString);

    if (!command.isEmpty()) {
        qDebug() << "[InputWidget::functionKeyPressed] Using configured hotkey command:" << command;
        sendCommandWithSeparator(command);
    } else {
        qDebug() << "[InputWidget::functionKeyPressed] No hotkey configured, sending literal:"
                 << fullKeyString;
        sendCommandWithSeparator(fullKeyString);
    }
}

bool InputWidget::numpadKeyPressed(int key, Qt::KeyboardModifiers modifiers)
{
    // Reuse the helper function to avoid duplicate switch statements
    QString keyName = getNumpadKeyName(key);
    if (keyName.isEmpty()) {
        qDebug() << "[InputWidget::numpadKeyPressed] Unknown numpad key:" << key;
        return false;
    }

    // Build the full key string with modifiers in canonical order: CTRL, SHIFT, ALT, META
    QString fullKeyString = buildHotkeyString(keyName, modifiers);

    qDebug() << "[InputWidget::numpadKeyPressed] Numpad key pressed:" << fullKeyString;

    // Check if there's a configured hotkey for this numpad key
    const QString command = getConfig().hotkeyManager.getCommand(fullKeyString);

    if (!command.isEmpty()) {
        qDebug() << "[InputWidget::numpadKeyPressed] Using configured hotkey command:" << command;
        sendCommandWithSeparator(command);
        return true;
    } else {
        qDebug() << "[InputWidget::numpadKeyPressed] No hotkey configured for:" << fullKeyString;
        return false;
    }
}

bool InputWidget::navigationKeyPressed(int key, Qt::KeyboardModifiers modifiers)
{
    // Reuse the helper function to avoid duplicate switch statements
    QString keyName = getNavigationKeyName(key);
    if (keyName.isEmpty()) {
        qDebug() << "[InputWidget::navigationKeyPressed] Unknown navigation key:" << key;
        return false;
    }

    // Build the full key string with modifiers in canonical order: CTRL, SHIFT, ALT, META
    QString fullKeyString = buildHotkeyString(keyName, modifiers);

    qDebug() << "[InputWidget::navigationKeyPressed] Navigation key pressed:" << fullKeyString;

    // Check if there's a configured hotkey for this key
    const QString command = getConfig().hotkeyManager.getCommand(fullKeyString);

    if (!command.isEmpty()) {
        qDebug() << "[InputWidget::navigationKeyPressed] Using configured hotkey command:" << command;
        sendCommandWithSeparator(command);
        return true;
    } else {
        qDebug() << "[InputWidget::navigationKeyPressed] No hotkey configured for:" << fullKeyString;
        return false;
    }
}

QString InputWidget::buildHotkeyString(const QString &keyName, Qt::KeyboardModifiers modifiers)
{
    QStringList parts;

    if (modifiers & Qt::ControlModifier) {
        parts << "CTRL";
    }
    if (modifiers & Qt::ShiftModifier) {
        parts << "SHIFT";
    }
    if (modifiers & Qt::AltModifier) {
        parts << "ALT";
    }
    if (modifiers & Qt::MetaModifier) {
        parts << "META";
    }

    parts << keyName;
    return parts.join("+");
}

bool InputWidget::arrowKeyPressed(const int key, Qt::KeyboardModifiers modifiers)
{
    // UP/DOWN with no modifiers cycle through command history
    if (modifiers == Qt::NoModifier) {
        if (key == Qt::Key_Up) {
            backwardHistory();
            return true;
        } else if (key == Qt::Key_Down) {
            forwardHistory();
            return true;
        }
    }

    // Arrow keys with modifiers check for hotkeys
    QString keyName;
    switch (key) {
    case Qt::Key_Up:
        keyName = "UP";
        break;
    case Qt::Key_Down:
        keyName = "DOWN";
        break;
    case Qt::Key_Left:
        keyName = "LEFT";
        break;
    case Qt::Key_Right:
        keyName = "RIGHT";
        break;
    default:
        return false;
    }

    QString fullKeyString = buildHotkeyString(keyName, modifiers);
    qDebug() << "[InputWidget::arrowKeyPressed] Arrow key pressed:" << fullKeyString;

    const QString command = getConfig().hotkeyManager.getCommand(fullKeyString);

    if (!command.isEmpty()) {
        qDebug() << "[InputWidget::arrowKeyPressed] Using configured hotkey command:" << command;
        sendCommandWithSeparator(command);
        return true;
    }

    // Let default behavior handle bare arrow keys (cursor movement)
    return false;
}

bool InputWidget::miscKeyPressed(int key, Qt::KeyboardModifiers modifiers)
{
    // Reuse the helper function to avoid duplicate switch statements
    QString keyName = getMiscKeyName(key);
    if (keyName.isEmpty()) {
        qDebug() << "[InputWidget::miscKeyPressed] Unknown misc key:" << key;
        return false;
    }

    QString fullKeyString = buildHotkeyString(keyName, modifiers);

    qDebug() << "[InputWidget::miscKeyPressed] Misc key pressed:" << fullKeyString;

    // Check if there's a configured hotkey for this key
    const QString command = getConfig().hotkeyManager.getCommand(fullKeyString);

    if (!command.isEmpty()) {
        qDebug() << "[InputWidget::miscKeyPressed] Using configured hotkey command:" << command;
        sendCommandWithSeparator(command);
        return true;
    } else {
        qDebug() << "[InputWidget::miscKeyPressed] No hotkey configured for:" << fullKeyString;
        return false;
    }
}

bool InputWidget::handleTerminalShortcut(int key)
{
    switch (key) {
    case Qt::Key_H: // ^H = backspace
        textCursor().deletePreviousChar();
        return true;

    case Qt::Key_U: // ^U = delete line (clear the input)
        base::clear();
        return true;

    case Qt::Key_W: // ^W = delete word (whitespace-delimited)
    {
        QTextCursor cursor = textCursor();
        // If at start, nothing to delete
        if (cursor.atStart()) {
            return true;
        }
        // First, skip any trailing whitespace before the word
        while (!cursor.atStart() && document()->characterAt(cursor.position() - 1).isSpace()) {
            cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);
        }
        // Then, select the word (non-whitespace characters)
        while (!cursor.atStart() && !document()->characterAt(cursor.position() - 1).isSpace()) {
            cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);
        }
        cursor.removeSelectedText();
        setTextCursor(cursor);
        return true;
    }
    }
    return false;
}

bool InputWidget::handleBasicKey(int key)
{
    switch (key) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
        gotInput();
        return true;

    case Qt::Key_Tab:
        return tryHistory(key);
    }
    return false;
}

bool InputWidget::handlePageKey(int key, Qt::KeyboardModifiers modifiers)
{
    // PageUp/PageDown without modifiers scroll the display widget
    if (modifiers == Qt::NoModifier) {
        bool isPageUp = (key == Qt::Key_PageUp);
        qDebug() << "[InputWidget::handlePageKey]" << (isPageUp ? "PageUp" : "PageDown");
        m_outputs.scrollDisplay(isPageUp);
        return true;
    }

    // With modifiers, check for hotkeys
    QString keyName = (key == Qt::Key_PageUp) ? "PAGEUP" : "PAGEDOWN";
    QString fullKeyString = buildHotkeyString(keyName, modifiers);

    qDebug() << "[InputWidget::handlePageKey] Page key with modifiers:" << fullKeyString;

    const QString command = getConfig().hotkeyManager.getCommand(fullKeyString);
    if (!command.isEmpty()) {
        qDebug() << "[InputWidget::handlePageKey] Using configured hotkey command:" << command;
        sendCommandWithSeparator(command);
        return true;
    }

    return false;
}

bool InputWidget::tryHistory(const int key)
{
    QTextCursor cursor = textCursor();
    switch (key) {
    case Qt::Key_Up:
        if (!cursor.movePosition(QTextCursor::Up, QTextCursor::MoveAnchor)) {
            // At the top of the document
            backwardHistory();
            return true;
        }
        break;
    case Qt::Key_Down:
        if (!cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor)) {
            // At the end of the document
            forwardHistory();
            return true;
        }
        break;
    case Qt::Key_Tab:
        tabComplete();
        return true;
    }
    return false;
}

// Process _hotkey commands and return true if handled
static bool processHotkeyCommand(const QString &input, InputWidgetOutputs &outputs)
{
    if (!input.startsWith("_hotkey")) {
        return false;
    }

    auto &hotkeyManager = setConfig().hotkeyManager;
    QString output;

    // Parse the command
    QStringList parts = input.split(' ', Qt::SkipEmptyParts);

    if (parts.size() == 1) {
        // _hotkey - show help (same as _hotkey help)
        output = "\nHotkey commands:\n"
                 "  _hotkey              - Show this help\n"
                 "  _hotkey config       - List all configured hotkeys\n"
                 "  _hotkey keys         - List available key names and modifiers\n"
                 "  _hotkey reset        - Reset hotkeys to defaults\n"
                 "  _hotkey KEY cmd      - Set a hotkey (e.g., _hotkey NUMPAD8 north)\n"
                 "  _hotkey KEY          - Remove a hotkey (e.g., _hotkey NUMPAD8)\n";
    } else if (parts.size() == 2) {
        QString arg = parts[1];

        if (arg.compare("help", Qt::CaseInsensitive) == 0) {
            // _hotkey help - same as _hotkey
            output = "\nHotkey commands:\n"
                     "  _hotkey              - Show this help\n"
                     "  _hotkey config       - List all configured hotkeys\n"
                     "  _hotkey keys         - List available key names and modifiers\n"
                     "  _hotkey reset        - Reset hotkeys to defaults\n"
                     "  _hotkey KEY cmd      - Set a hotkey (e.g., _hotkey NUMPAD8 north)\n"
                     "  _hotkey KEY          - Remove a hotkey (e.g., _hotkey NUMPAD8)\n";
        } else if (arg.compare("config", Qt::CaseInsensitive) == 0) {
            // _hotkey config - list all configured hotkeys in their saved order
            const auto &hotkeys = hotkeyManager.getAllHotkeys();
            if (hotkeys.empty()) {
                output = "\nNo hotkeys configured.\n";
            } else {
                output = "\nConfigured hotkeys:\n";
                for (const auto &[key, command] : hotkeys) {
                    output += QString("  %1 = %2\n").arg(key, -20).arg(command);
                }
            }
        } else if (arg.compare("keys", Qt::CaseInsensitive) == 0) {
            // _hotkey keys
            output = "\nAvailable key names:\n"
                     "  Function keys: F1-F12\n"
                     "  Numpad: NUMPAD0-9, NUMPAD_SLASH, NUMPAD_ASTERISK,\n"
                     "          NUMPAD_MINUS, NUMPAD_PLUS, NUMPAD_PERIOD\n"
                     "  Navigation: HOME, END, INSERT\n"
                     "  Misc: ACCENT, 0-9, HYPHEN, EQUAL\n"
                     "\n"
                     "Available modifiers:\n"
                     "  CTRL, SHIFT, ALT, META\n"
                     "\n"
                     "Example: CTRL+SHIFT+F1, ALT+NUMPAD8\n";
        } else if (arg.compare("reset", Qt::CaseInsensitive) == 0) {
            // _hotkey reset - reset to defaults
            hotkeyManager.resetToDefaults();
            output = "\nHotkeys reset to defaults.\n";
        } else {
            // _hotkey KEY - remove a hotkey
            hotkeyManager.removeHotkey(arg);
            output = QString("\nHotkey removed: %1\n").arg(arg.toUpper());
        }
    } else {
        // _hotkey KEY command - set a hotkey
        QString key = parts[1];
        QString command = parts.mid(2).join(' ');
        hotkeyManager.setHotkey(key, command);
        output = QString("\nHotkey set: %1 = %2\n").arg(key.toUpper()).arg(command);
    }

    outputs.displayMessage(output);
    return true;
}

// Process _config commands and return true if handled
static bool processConfigCommand(const QString &input, InputWidgetOutputs &outputs)
{
    if (!input.startsWith("_config")) {
        return false;
    }

    QString output;

    // Parse the command
    QStringList parts = input.split(' ', Qt::SkipEmptyParts);

    if (parts.size() == 1) {
        // _config - show help
        output = "\nConfiguration commands:\n"
                 "  _config              - Show this help\n"
                 "  _config edit         - Open all config in editor\n"
                 "  _config edit hotkey  - Open hotkey config in editor\n"
                 "\n";
    } else if (parts.size() >= 2 && parts[1].compare("edit", Qt::CaseInsensitive) == 0) {
        // _config edit [section]
        QString section = (parts.size() >= 3) ? parts[2].toLower() : "all";

        if (section == "all" || section == "hotkey" || section == "hotkeys") {
            // Serialize current hotkeys using HotkeyManager
            QString content = getConfig().hotkeyManager.exportToCliFormat();

            // Create the editor widget
            auto *editor = new RemoteEditWidget(true, // editSession = true (editable)
                                                "MMapper Configuration - Hotkeys",
                                                content,
                                                nullptr);

            // Connect save signal to import the edited content
            QObject::connect(editor, &RemoteEditWidget::sig_save, [&outputs](const QString &edited) {
                int count = setConfig().hotkeyManager.importFromCliFormat(edited);
                outputs.displayMessage(QString("\n%1 hotkeys imported.\n").arg(count));
            });

            // Show the editor
            editor->setAttribute(Qt::WA_DeleteOnClose);
            editor->show();
            editor->activateWindow();

            output = "\nOpening configuration editor...\n";
        } else {
            output = QString("\nUnknown config section: %1\n"
                             "Available sections: hotkey\n")
                         .arg(section);
        }
    } else {
        output = "\nUnknown config command. Type '_config' for help.\n";
    }

    outputs.displayMessage(output);
    return true;
}

void InputWidget::sendCommandWithSeparator(const QString &command)
{
    const auto &settings = getConfig().integratedClient;

    // Handle command separator (e.g., "l;;look" sends "l" then "look")
    if (settings.useCommandSeparator && !settings.commandSeparator.isEmpty()) {
        const QString &sep = settings.commandSeparator;
        const QString escaped = QRegularExpression::escape(sep);
        const QRegularExpression regex(QString("(?<!\\\\)%1").arg(escaped));
        const QStringList commands = command.split(regex);
        for (QString cmd : commands) {
            cmd.replace("\\" + sep, sep);
            sendUserInput(cmd);
        }
    } else {
        sendUserInput(command);
    }
}

void InputWidget::gotInput()
{
    const auto &settings = getConfig().integratedClient;

    QString input = toPlainText();
    if (settings.clearInputOnEnter) {
        clear();
    } else {
        selectAll();
    }

    // Check for _hotkey command
    if (processHotkeyCommand(input, m_outputs)) {
        m_inputHistory.addInputLine(input);
        m_tabHistory.addInputLine(input);
        return;
    }

    // Check for _config command
    if (processConfigCommand(input, m_outputs)) {
        m_inputHistory.addInputLine(input);
        m_tabHistory.addInputLine(input);
        return;
    }

    // Send input (with command separator handling if enabled)
    sendCommandWithSeparator(input);

    m_inputHistory.addInputLine(input);
    m_tabHistory.addInputLine(input);
}

void InputHistory::addInputLine(const QString &string)
{
    if (!string.isEmpty() && (empty() || back() != string)) {
        // Add to line history if it is a new entry
        push_front(string);
    }

    // Trim line history
    if (static_cast<int>(size()) > getConfig().integratedClient.linesOfInputHistory) {
        pop_back();
    }

    // Reset the iterator
    m_iterator = begin();
}

void TabHistory::addInputLine(const QString &string)
{
    QStringList list = string.split(g_whitespaceRx, Qt::SplitBehaviorFlags::SkipEmptyParts);
    for (const QString &word : list) {
        if (word.length() > MIN_WORD_LENGTH) {
            // Adding this word to the dictionary
            push_front(word);

            // Trim dictionary
            if (static_cast<int>(size())
                > getConfig().integratedClient.tabCompletionDictionarySize) {
                pop_back();
            }
        }
    }

    // Reset the iterator
    m_iterator = begin();
}

void InputWidget::forwardHistory()
{
    clear();
    if (m_inputHistory.atFront()) {
        m_outputs.showMessage("Reached beginning of input history", 1000);
        return;
    }

    if (m_inputHistory.atEnd()) {
        m_inputHistory.backward();
    }

    if (!m_inputHistory.atFront()) {
        m_inputHistory.backward();
        insertPlainText(m_inputHistory.value());
    }
}

void InputWidget::backwardHistory()
{
    if (m_inputHistory.atEnd()) {
        m_outputs.showMessage("Reached end of input history", 1000);
        return;
    }

    clear();
    insertPlainText(m_inputHistory.value());
    if (!m_inputHistory.atEnd()) {
        m_inputHistory.forward();
    }
}

void InputWidget::tabComplete()
{
    // Select all characters up until the previous whitespace
    QTextCursor current = textCursor();
    if (m_tabHistory.empty() || current.atStart()
        || document()->characterAt(current.selectionStart() - 1).isSpace()) {
        return;
    }
    do {
        current.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);
    } while (!current.atStart() && !document()->characterAt(current.selectionStart() - 1).isSpace());
    if (current.selectedText().isEmpty()) {
        return;
    }
    if (!m_tabbing) {
        m_tabFragment = current.selectedText();
        m_tabHistory.reset();
        m_tabbing = true;
    }

    // Iterate through all previous words
    while (!m_tabHistory.atEnd()) {
        const auto &word = m_tabHistory.value();
        if (!word.startsWith(m_tabFragment)) {
            // Try the next word
            m_tabHistory.forward();
            continue;
        }

        // Found a previous word to complete to
        current.removeSelectedText();
        current.movePosition(QTextCursor::NextWord, QTextCursor::KeepAnchor);
        current.insertText(word);
        auto length = static_cast<int>(word.length() - m_tabFragment.length());
        current.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, length);
        current.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, length);
        setTextCursor(current);

        m_tabHistory.forward();
        break;
    }

    // If we reach the end then loop back to the beginning and clear the selected text again
    if (m_tabHistory.atEnd()) {
        textCursor().removeSelectedText();
        m_tabHistory.reset();
    }
}

bool InputWidget::event(QEvent *const event)
{
    if (event->type() == QEvent::ShortcutOverride) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        auto classification = classifyKey(keyEvent->key(), keyEvent->modifiers());

        if (classification.shouldHandle) {
            qDebug() << "[InputWidget::event] ShortcutOverride - Key:" << keyEvent->key()
                     << "Type:" << static_cast<int>(classification.type);

            // Handle directly if there are real modifiers (some don't generate KeyPress)
            if (classification.realModifiers != Qt::NoModifier) {
                bool handled = false;

                switch (classification.type) {
                case KeyType::FunctionKey:
                    functionKeyPressed(classification.keyName, classification.realModifiers);
                    handled = true;
                    break;
                case KeyType::NumpadKey:
                    handled = numpadKeyPressed(keyEvent->key(), classification.realModifiers);
                    break;
                case KeyType::NavigationKey:
                    handled = navigationKeyPressed(keyEvent->key(), classification.realModifiers);
                    break;
                case KeyType::ArrowKey:
                    handled = arrowKeyPressed(keyEvent->key(), classification.realModifiers);
                    break;
                case KeyType::MiscKey:
                    handled = miscKeyPressed(keyEvent->key(), classification.realModifiers);
                    break;
                case KeyType::PageKey:
                    handled = handlePageKey(keyEvent->key(), classification.realModifiers);
                    break;
                case KeyType::TerminalShortcut:
                case KeyType::BasicKey:
                case KeyType::Other:
                    break;
                }

                if (handled) {
                    m_handledInShortcutOverride = true;
                    event->accept();
                    return true;
                }
            }

            // Accept so KeyPress comes through
            event->accept();
            return true;
        }
    }

    m_paletteManager.tryUpdateFromFocusEvent(*this, deref(event).type());
    return QPlainTextEdit::event(event);
}
