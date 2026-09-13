// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "displaywidget.h"

#include "../configuration/configuration.h"
#include "../global/AnsiTextUtils.h"
#include "../global/window_utils.h"

#include <memory>

#include <QApplication>
#include <QMessageLogContext>
#include <QRegularExpression>
#include <QScrollBar>
#include <QScroller>
#include <QString>
#include <QStyle>
#include <QTextCursor>
#include <QTimer>
#include <QToolTip>
#include <QtGui>
#include <QtWidgets>

namespace { // anonymous

const constexpr int TAB_WIDTH_SPACES = 8;

void foreach_char(const QChar qchar,
                  const QStringView text,
                  const std::function<void()> &callback_qchar,
                  const std::function<void(const QStringView nonQchar)> &callback_between)
{
    qsizetype pos = 0;
    const qsizetype end = text.length();
    while (pos != end) {
        const qsizetype qcharIndex = text.indexOf(qchar, pos);
        if (qcharIndex < 0) {
            callback_between(text.mid(pos));
            break;
        }

        callback_between(text.mid(pos, qcharIndex - pos));
        callback_qchar();
        pos = qcharIndex + 1;
    }
}

} // namespace

FontDefaults::FontDefaults()
{
    const auto &settings = getConfig().integratedClient;

    // Default Colors
    defaultBg = settings.backgroundColor;
    defaultFg = settings.foregroundColor;

    // Default Font
    serverOutputFont.fromString(settings.font);
}

void AnsiTextHelper::init()
{
    // core.init() sets the frame and character format's background/foreground colors from
    // defaults; the font family is a widget-only concern, so it's applied here afterward.
    core.init();
    core.format.setFont(defaults.serverOutputFont);
    core.cursor.setCharFormat(core.format);
}

DisplayWidgetOutputs::~DisplayWidgetOutputs() = default;
// Touch long-press duration; see mousePressEvent().
static constexpr int LONG_PRESS_MS = 600;

DisplayWidget::DisplayWidget(QWidget *const parent)
    : QTextBrowser(parent)
    , m_ansiTextHelper{static_cast<QTextEdit &>(*this)}
    , m_timer{new QTimer(this)}
{
    setReadOnly(true);
    setOverwriteMode(true);
    setUndoRedoEnabled(false);

    m_timer->setSingleShot(true);
    connect(m_timer, &QTimer::timeout, this, [this]() {
        QTextFrameFormat frameFormat = document()->rootFrame()->frameFormat();
        frameFormat.setBackground(getConfig().integratedClient.backgroundColor);
        document()->rootFrame()->setFrameFormat(frameFormat);
    });
    m_longPressTimer.setSingleShot(true);
    m_longPressTimer.setInterval(LONG_PRESS_MS);
    connect(&m_longPressTimer, &QTimer::timeout, this, [this]() {
        if (!m_longPressPos) {
            return;
        }
        const QPoint pos = *m_longPressPos;
        m_longPressPos.reset();
        mmqt::popupMenu(std::unique_ptr<QMenu>{createStandardContextMenu(pos)},
                        viewport()->mapToGlobal(pos));
    });

    setDocumentTitle("MMapper Mud Client");
    setTextInteractionFlags(Qt::TextBrowserInteraction);
    setOpenExternalLinks(true);
    setTabChangesFocus(false);

    // REVISIT: Is this necessary to do in both places?
    document()->setUndoRedoEnabled(false);
    m_ansiTextHelper.init();

    // Set word wrap mode and other settings
    QFontMetrics fm{getDefaultFont()};
    setLineWrapMode(QTextEdit::FixedColumnWidth);
    setWordWrapMode(QTextOption::WordWrap);
    setSizeIncrement(fm.averageCharWidth(), fm.lineSpacing());
    setTabStopDistance(fm.horizontalAdvance(" ") * TAB_WIDTH_SPACES);

    // Scrollbar settings
    QScrollBar *const scrollbar = verticalScrollBar();
    scrollbar->setSingleStep(fm.lineSpacing());
    scrollbar->setPageStep(sizeHint().height());
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    connect(this, &DisplayWidget::copyAvailable, this, [this](const bool available) {
        m_canCopy = available;
        if (available) {
            this->setFocus();
        }
    });

    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, [this](int value) {
        bool atBottom = (value == verticalScrollBar()->maximum());
        getOutput().showPreview(!atBottom);
    });
}

DisplayWidget::~DisplayWidget() = default;

QSize DisplayWidget::sizeHint() const
{
    const auto &settings = getConfig().integratedClient;
    const auto &margins = contentsMargins();
    const int frame = frameWidth() * 2;
    QFontMetrics fm{getDefaultFont()};
    const int scrollbarExtent = style()->pixelMetric(QStyle::PM_ScrollBarExtent);

    int x = (settings.columns * fm.averageCharWidth()) + margins.left() + margins.right()
            + scrollbarExtent + frame;
    int y = (settings.rows * fm.lineSpacing()) + margins.top() + margins.bottom() + scrollbarExtent
            + frame;

    return QSize(x, y);
}

void DisplayWidget::mousePressEvent(QMouseEvent *const event)
{
    m_longPressTimer.stop();
    m_longPressPos.reset();
    m_touchPressPos.reset();
    m_touchScrolling = false;

    // A single finger (delivered as a synthesized left press) scrolls the
    // output rather than selecting text, and a held finger opens the
    // context menu, since touch never synthesizes a right-click. The press
    // is not passed on, so no selection starts; the mouse is unaffected.
    if (event->button() == Qt::LeftButton && event->source() != Qt::MouseEventNotSynthesized) {
        const QPoint pos = event->position().toPoint();
        m_longPressPos = pos;
        m_longPressTimer.start();
        m_touchPressPos = pos;
        QScroller::scroller(viewport())
            ->handleInput(QScroller::InputPress,
                          event->position(),
                          static_cast<qint64>(event->timestamp()));
        event->accept();
        return;
    }
    QTextBrowser::mousePressEvent(event);
}

void DisplayWidget::mouseMoveEvent(QMouseEvent *const event)
{
    if (m_touchPressPos) {
        if (!m_touchScrolling
            && (event->position().toPoint() - *m_touchPressPos).manhattanLength()
                   > QApplication::startDragDistance()) {
            m_touchScrolling = true;
            m_longPressTimer.stop();
            m_longPressPos.reset();
        }
        QScroller::scroller(viewport())
            ->handleInput(QScroller::InputMove,
                          event->position(),
                          static_cast<qint64>(event->timestamp()));
        event->accept();
        return;
    }
    QTextBrowser::mouseMoveEvent(event);
}

void DisplayWidget::mouseReleaseEvent(QMouseEvent *const event)
{
    m_longPressTimer.stop();
    m_longPressPos.reset();
    if (m_touchPressPos) {
        m_touchPressPos.reset();
        m_touchScrolling = false;
        QScroller::scroller(viewport())
            ->handleInput(QScroller::InputRelease,
                          event->position(),
                          static_cast<qint64>(event->timestamp()));
        event->accept();
        return;
    }
    QTextBrowser::mouseReleaseEvent(event);
}

void DisplayWidget::resizeEvent(QResizeEvent *const event)
{
    const auto &margins = contentsMargins();
    QFontMetrics fm{getDefaultFont()};
    const int scrollbarExtent = style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    const int frame = frameWidth() * 2;

    int widthAvailable = size().width() - margins.left() - margins.right() - scrollbarExtent
                         - frame;
    int heightAvailable = size().height() - margins.top() - margins.bottom() - scrollbarExtent
                          - frame;

    int x = widthAvailable / fm.averageCharWidth();
    int y = heightAvailable / fm.lineSpacing();

    // REVISIT: Right now only Mac is correct and we need to replace these row hacks
    if constexpr (CURRENT_PLATFORM == PlatformEnum::Linux) {
        y -= 6;
    } else if constexpr (CURRENT_PLATFORM == PlatformEnum::Windows) {
        y -= 2;
    }

    // Set the line wrap width/height
    setLineWrapColumnOrWidth(x);
    verticalScrollBar()->setPageStep(y);

    // Inform user of new dimensions
    QString message = QString("Dimensions: %1x%2").arg(x).arg(y);
    QFontMetrics fmToolTip(QToolTip::font());
    QPoint messageDiff(fmToolTip.horizontalAdvance(message) / 2, fmToolTip.lineSpacing() / 2);
    QToolTip::showText(mapToGlobal(rect().center() - messageDiff), message, this, rect(), 1000);

    const auto &settings = getConfig().integratedClient;
    if (settings.autoResizeTerminal) {
        getOutput().windowSizeChanged(x, y);
    }

    QTextEdit::resizeEvent(event);

    bool atBottom = (verticalScrollBar()->sliderPosition() == verticalScrollBar()->maximum());
    getOutput().showPreview(!atBottom);
}

void DisplayWidget::keyPressEvent(QKeyEvent *event)
{
    bool isModifier = event->modifiers() != Qt::NoModifier;
    auto isNavigationKey = [&event]() {
        switch (event->key()) {
        case Qt::Key_PageUp:
        case Qt::Key_PageDown:
        case Qt::Key_Home:
        case Qt::Key_End:
        case Qt::Key_Left:
        case Qt::Key_Right:
        case Qt::Key_Up:
        case Qt::Key_Down:
            return true;
        default:
            return false;
        }
    };
    auto isAllowedKeySequence = [&event]() {
        return event->matches(QKeySequence::Copy) || event->matches(QKeySequence::Paste)
               || event->matches(QKeySequence::Cut) || event->matches(QKeySequence::SelectAll);
    };

    if (isModifier || isNavigationKey() || isAllowedKeySequence()) {
        QTextBrowser::keyPressEvent(event);
    } else {
        getOutput().returnFocusToInput();
        event->accept();
    }
}

void DisplayWidget::slot_displayText(const QStringView str)
{
    const int lineLimit = getConfig().integratedClient.linesOfScrollback;

    auto &vscroll = deref(verticalScrollBar());
    constexpr int ALMOST_ALL_THE_WAY = 4;
    const bool wasAtBottom = (vscroll.sliderPosition() >= vscroll.maximum() - ALMOST_ALL_THE_WAY);

    auto on_bell = [this]() {
        const auto &settings = getConfig().integratedClient;
        if (settings.audibleBell) {
            QApplication::beep();
        }
        if (settings.visualBell) {
            auto setBackgroundColor = [this](const QColor &color) {
                QTextFrameFormat frameFormat = document()->rootFrame()->frameFormat();
                frameFormat.setBackground(color);
                document()->rootFrame()->setFrameFormat(frameFormat);
            };

            QColor flashColor = getConfig().integratedClient.backgroundColor;
            flashColor.setRed(std::min(255, flashColor.red() + 80));
            setBackgroundColor(flashColor);

            m_timer->start(250);
        }
    };

    foreach_char(mmqt::QC_ALERT, str, on_bell, [this](const QStringView nonBellText) {
        m_ansiTextHelper.displayText(nonBellText);
    });

    // REVISIT: include option to limit scrollback in the displayText function,
    // so it can choose to remove lines from the top when the overflow occurs,
    // to improve performance for massive inserts.
    m_ansiTextHelper.limitScrollback(lineLimit);

    // Detecting the keyboard Scroll Lock status would be preferable, but we'll have to live with
    // this because Qt is apparently the only windowing system in existence that doesn't provide
    // a way to query CapsLock/NumLock/ScrollLock ?!?
    //
    // REVISIT: should this be user-configurable?
    if (wasAtBottom) {
        vscroll.setSliderPosition(vscroll.maximum());
    }
}

void setAnsiText(QTextEdit *const pEdit, const std::string_view text)
{
    QTextEdit &edit = deref(pEdit);

    edit.clear();
    edit.setReadOnly(true);
    edit.setOverwriteMode(true);
    edit.setUndoRedoEnabled(false);
    edit.setTextInteractionFlags(Qt::TextSelectableByMouse);

    // REVISIT: Is this necessary to do in both places?
    deref(edit.document()).setUndoRedoEnabled(false);

    AnsiTextHelper helper{edit};
    helper.init();
    helper.displayText(mmqt::toQStringUtf8(text));

    // FIXME: why don't the scroll bars work?
    if (QScrollBar *const vs = edit.verticalScrollBar()) {
        vs->setEnabled(true);
    }
}
