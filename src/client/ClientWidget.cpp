// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "ClientWidget.h"

#include "../configuration/configuration.h"
#include "../global/AnsiOstream.h"
#include "../global/ConfigConsts-Computed.h"
#include "../proxy/connectionlistener.h"
#include "ClientTelnet.h"
#include "HotkeyManager.h"
#include "PreviewWidget.h"
#include "displaywidget.h"
#include "inputwidget.h"
#include "stackedinputwidget.h"
#include "ui_ClientWidget.h"

#include <algorithm>
#include <memory>

#include <QDateTime>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QScrollBar>
#include <QSplitter>
#include <QString>
#include <QTimer>
#include <QToolButton>

ClientWidget::ClientWidget(ConnectionListener &listener,
                           HotkeyManager &hotkeyManager,
                           QWidget *const parent)
    : QWidget(parent)
    , m_listener{listener}
    , m_hotkeyManager{hotkeyManager}
{
    setWindowTitle("MMapper Client");

    initPipeline();

    auto &ui = getUi();
    initWelcomePage();

    ui.input->installEventFilter(this);
    ui.display->setFocusPolicy(Qt::TabFocus);

    // The card asks how to play; the preference can answer instead, and
    // the web build has only the built-in client.
    switch (getConfig().general.gameClient) {
    case GameClientEnum::BUILT_IN:
        play();
        break;
    case GameClientEnum::EXTERNAL:
        ui.parent->setCurrentWidget(ui.externalPage);
        break;
    case GameClientEnum::ASK:
        ui.parent->setCurrentWidget(ui.welcomePage);
        ui.playButton->setFocus();
        break;
    }
}

void ClientWidget::initWelcomePage()
{
    auto &ui = getUi();
    const auto &audio = getConfig().audio;
    const QString port = QString::number(getConfig().connection.localPort);
    ui.portLabel->setText(tr("Point it at localhost, port %1").arg(port));
    ui.waitingLabel->setText(tr("Waiting for your MUD client on localhost, port %1").arg(port));
    ui.soundCheckBox->setChecked(audio.getMusicVolume() > 0 || audio.getSoundVolume() > 0);

    // What the card decided: sound, and (if asked not to ask again) how to
    // play from now on. Either remembered choice is undone in Preferences.
    const auto applyChoices = [this](const GameClientEnum client) {
        auto &ui2 = getUi();
        // The box is a coarse switch over both channels (the sliders in
        // Preferences > Audio set levels): on brings a channel at 0 up to
        // the default level and unlocks; off is both at 0.
        auto &settings = setConfig().audio;
        using AudioSettings = Configuration::AudioSettings;
        if (ui2.soundCheckBox->isChecked()) {
            if (settings.getMusicVolume() == 0) {
                settings.setMusicVolume(AudioSettings::DEFAULT_VOLUME);
            }
            if (settings.getSoundVolume() == 0) {
                settings.setSoundVolume(AudioSettings::DEFAULT_VOLUME);
            }
            settings.setUnlocked();
        } else {
            settings.setMusicVolume(0);
            settings.setSoundVolume(0);
        }
        if (ui2.dontAskCheckBox->isChecked()) {
            setConfig().general.gameClient = client;
        }
    };
    connect(ui.playButton, &QAbstractButton::clicked, this, [this, applyChoices]() {
        applyChoices(GameClientEnum::BUILT_IN);
        play();
    });
    connect(ui.externalButton, &QAbstractButton::clicked, this, [this, applyChoices]() {
        applyChoices(GameClientEnum::EXTERNAL);
        getUi().parent->setCurrentWidget(getUi().externalPage);
    });
    connect(ui.playInsteadButton, &QAbstractButton::clicked, this, [this]() { play(); });
}

void ClientWidget::play()
{
    auto &ui = getUi();
    ui.parent->setCurrentWidget(ui.clientPage);
    getTelnet().connectToHost(m_listener);
}

ClientWidget::~ClientWidget() = default;

ClientWidget::Pipeline::~Pipeline()
{
    objs.clientTelnet.reset();
    objs.ui.reset();
}

QSize ClientWidget::sizeHint() const
{
    // Preferred size is the configured terminal columns/rows; the minimum is
    // left to the layout so the dock can shrink on small screens.
    return m_pipeline.objs.ui->display->sizeHint();
}

void ClientWidget::initPipeline()
{
    m_pipeline.objs.ui = std::make_unique<Ui::ClientWidget>();
    getUi().setupUi(this); // creates stacked input and display

    initStackedInputWidget();
    initTouchInputStrip();
    initDisplayWidget();

    initClientTelnet();
}

void ClientWidget::initStackedInputWidget()
{
    class NODISCARD LocalStackedInputWidgetOutputs final : public StackedInputWidgetOutputs
    {
    private:
        ClientWidget &m_self;

    public:
        explicit LocalStackedInputWidgetOutputs(ClientWidget &self)
            : m_self{self}
        {}

    private:
        NODISCARD ClientWidget &getSelf() { return m_self; }
        NODISCARD ClientTelnet &getTelnet() { return getSelf().getTelnet(); }
        NODISCARD DisplayWidget &getDisplay() { return getSelf().getDisplay(); }
        NODISCARD PreviewWidget &getPreview() { return getSelf().getPreview(); }

    private:
        void virt_sendUserInput(const QString &msg) final
        {
            if (!getTelnet().isConnected()) {
                getTelnet().connectToHost(getSelf().m_listener);
            } else {
                getTelnet().sendToMud(msg);
            }
        }

        void virt_displayMessage(const QString &msg) final
        {
            getDisplay().slot_displayText(msg);
            getPreview().displayText(msg);
        }

        void virt_showMessage(const QString &msg, MAYBE_UNUSED int timeout) final
        {
            // REVISIT: Why is timeout ignored?
            getSelf().slot_onShowMessage(msg);
        }
        void virt_requestPassword() final { getSelf().getInput().requestPassword(); }
        void virt_scrollDisplay(bool pageUp) final
        {
            if (auto *scrollBar = getDisplay().verticalScrollBar()) {
                int pageStep = scrollBar->pageStep();
                int delta = pageUp ? -pageStep : pageStep;
                scrollBar->setValue(scrollBar->value() + delta);
            }
        }

        std::optional<QString> virt_getHotkey(const Hotkey &hk) final
        {
            auto &hotkeys = getSelf().getHotkeys();
            if (auto cmd = hotkeys.getCommand(hk)) {
                return mmqt::toQStringUtf8(*cmd);
            }
            return std::nullopt;
        }
    };
    auto &out = m_pipeline.outputs.stackedInputWidgetOutputs;
    out = std::make_unique<LocalStackedInputWidgetOutputs>(*this);
    getInput().init(deref(out));
}

void ClientWidget::initTouchInputStrip()
{
    // On-screen keyboards have no Up/Down/Tab, so the compact layout shows a
    // row of buttons beside the input that drives command history and
    // completion (see setCompactLayout()). The input keeps its
    // place in the client page's splitter; this wraps it in a row.
    auto &ui = getUi();
    QSplitter &splitter = deref(ui.clientPage);
    const int inputIndex = splitter.indexOf(ui.input);

    auto *const row = new QWidget(this);
    auto *const layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(ui.input, 1);

    auto *const strip = new QWidget(row);
    auto *const stripLayout = new QHBoxLayout(strip);
    stripLayout->setContentsMargins(0, 0, 0, 0);
    stripLayout->setSpacing(0);

    InputWidget &inputWidget = getInput().getInputWidget();
    static constexpr int TOUCH_BUTTON_SIZE = 44; // logical px; the usual finger target
    const auto addButton = [&](const QString &text, const QString &toolTip, auto &&onClicked) {
        auto *const button = new QToolButton(strip);
        button->setText(text);
        button->setToolTip(toolTip);
        button->setAutoRaise(true);
        button->setFocusPolicy(Qt::NoFocus); // never take focus from the input
        button->setFixedSize(TOUCH_BUTTON_SIZE, TOUCH_BUTTON_SIZE);
        connect(button,
                &QToolButton::clicked,
                &inputWidget,
                std::forward<decltype(onClicked)>(onClicked));
        stripLayout->addWidget(button);
    };
    addButton(QStringLiteral("\u2191"), tr("Previous command"), &InputWidget::backwardHistory);
    addButton(QStringLiteral("\u2193"), tr("Next command"), &InputWidget::forwardHistory);
    addButton(QStringLiteral("\u21e5"), tr("Complete word"), &InputWidget::tabComplete);
    strip->hide();
    layout->addWidget(strip, 0, Qt::AlignBottom);

    splitter.insertWidget(inputIndex, row);
    splitter.setCollapsible(splitter.indexOf(row), false);
    m_touchInputStrip = strip;
}

void ClientWidget::setCompactLayout(const bool compact)
{
    if (m_touchInputStrip != nullptr) {
        m_touchInputStrip->setVisible(compact);
    }
    m_previewEnabled = !compact;
    if (!m_previewEnabled) {
        getPreview().hide();
    }
}

void ClientWidget::initDisplayWidget()
{
    class NODISCARD LocalDisplayWidgetOutputs final : public DisplayWidgetOutputs
    {
    private:
        ClientWidget &m_self;

    public:
        explicit LocalDisplayWidgetOutputs(ClientWidget &self)
            : m_self{self}
        {}

    private:
        NODISCARD ClientWidget &getSelf() { return m_self; }
        NODISCARD ClientTelnet &getTelnet() { return getSelf().getTelnet(); }

    private:
        void virt_showMessage(const QString &msg, int /*timeout*/) final
        {
            getSelf().slot_onShowMessage(msg);
        }
        void virt_windowSizeChanged(const int width, const int height) final
        {
            getTelnet().onWindowSizeChanged(width, height);
        }
        void virt_returnFocusToInput() final { getSelf().getInput().setFocus(); }
        void virt_showPreview(bool visible) final
        {
            getSelf().getPreview().setVisible(visible && getSelf().m_previewEnabled);
        }
    };
    auto &out = m_pipeline.outputs.displayWidgetOutputs;
    out = std::make_unique<LocalDisplayWidgetOutputs>(*this);
    getDisplay().init(deref(out));
}

void ClientWidget::initClientTelnet()
{
    class NODISCARD LocalClientTelnetOutputs final : public ClientTelnetOutputs
    {
    private:
        ClientWidget &m_self;

    public:
        explicit LocalClientTelnetOutputs(ClientWidget &self)
            : m_self{self}
        {}

    private:
        ClientWidget &getClient() { return m_self; }
        DisplayWidget &getDisplay() { return getClient().getDisplay(); }
        PreviewWidget &getPreview() { return getClient().getPreview(); }
        StackedInputWidget &getInput() { return getClient().getInput(); }

    private:
        void virt_connected() final
        {
            getClient().relayMessage("Connected using the integrated client");
            // Focus should be on the input
            getInput().setFocus();
        }
        void virt_disconnected() final
        {
            getClient().displayReconnectHint();
            getClient().relayMessage("Disconnected using the integrated client");
        }
        void virt_socketError(const QString &errorStr) final
        {
            getDisplay().slot_displayText(QString("\nInternal error! %1\n").arg(errorStr));
        }
        void virt_echoModeChanged(const bool echo) final
        {
            getInput().setEchoMode(echo ? EchoModeEnum::Visible : EchoModeEnum::Hidden);
        }

        void virt_sendToUser(const QString &str) final
        {
            getDisplay().slot_displayText(str);
            getPreview().displayText(str);

            // Re-open the password dialog if we get a message in hidden echo mode
            if (getClient().getInput().getEchoMode() == EchoModeEnum::Hidden) {
                getClient().getInput().requestPassword();
            }
        }
    };
    auto &out = m_pipeline.outputs.clientTelnetOutputs;
    out = std::make_unique<LocalClientTelnetOutputs>(*this);
    m_pipeline.objs.clientTelnet = std::make_unique<ClientTelnet>(deref(out));
}

DisplayWidget &ClientWidget::getDisplay()
{
    return deref(getUi().display);
}

PreviewWidget &ClientWidget::getPreview()
{
    return deref(getUi().preview);
}

StackedInputWidget &ClientWidget::getInput()
{
    return deref(getUi().input);
}

ClientTelnet &ClientWidget::getTelnet() // NOLINT (no, this shouldn't be const)
{
    return deref(m_pipeline.objs.clientTelnet);
}

HotkeyManager &ClientWidget::getHotkeys()
{
    return m_hotkeyManager;
}

void ClientWidget::slot_onVisibilityChanged(const bool /*visible*/)
{
    if (!isUsingClient()) {
        return;
    }

    // Delay connecting to verify that visibility is not just the dock popping back in
    QTimer::singleShot(500, [this]() {
        if (getTelnet().isConnected() && !isVisible()) {
            // Disconnect if the widget is closed or minimized
            getTelnet().disconnectFromHost();
        } else if (!getTelnet().isConnected() && isVisible()) {
            getInput().setFocus();
        }
    });
}

bool ClientWidget::isUsingClient() const
{
    const auto &ui = getUi();
    return ui.parent->currentWidget() == ui.clientPage;
}

void ClientWidget::displayReconnectHint()
{
    constexpr const auto whiteOnCyan = getRawAnsi(AnsiColor16Enum::white, AnsiColor16Enum::cyan);
    std::stringstream oss;
    AnsiOstream aos{oss};
    aos.writeWithColor(whiteOnCyan, "\n\n\nPress return to reconnect.\n");
    getDisplay().slot_displayText(mmqt::toQStringUtf8(oss.str()));
}

void ClientWidget::slot_onShowMessage(const QString &message)
{
    relayMessage(message);
}

void ClientWidget::slot_saveLog()
{
    const auto getDocStringUtf8 = [](const QTextDocument *const pDoc) -> QByteArray {
        auto &doc = deref(pDoc);
        const QString string = doc.toPlainText();
        return string.toUtf8();
    };
    const QByteArray logContent = getDocStringUtf8(getDisplay().document());
    QString newFileName = "log-" + QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss")
                          + ".txt";
    QFileDialog::saveFileContent(logContent, newFileName);
}

void ClientWidget::slot_saveLogAsHtml()
{
    const auto getDocStringUtf8 = [](const QTextDocument *const pDoc) -> QByteArray {
        auto &doc = deref(pDoc);
        const QString string = doc.toHtml();
        return string.toUtf8();
    };
    const QByteArray logContent = getDocStringUtf8(getDisplay().document());
    QString newFileNameHtml = "log-" + QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss")
                              + ".html";
    QFileDialog::saveFileContent(logContent, newFileNameHtml);
}

bool ClientWidget::focusNextPrevChild(MAYBE_UNUSED bool next)
{
    if (getInput().hasFocus()) {
        getDisplay().setFocus();
    } else {
        getInput().setFocus();
    }
    return true;
}
