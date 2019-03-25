/************************************************************************
**
** Authors:   Nils Schimmelmann <nschimme@gmail.com> (Jahara)
**
** This file is part of the MMapper project.
** Maintained by Nils Schimmelmann <nschimme@gmail.com>
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the:
** Free Software Foundation, Inc.
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
************************************************************************/

#include "remoteeditwidget.h"

#include <utility>
#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMessageLogContext>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QScopedPointer>
#include <QSize>
#include <QStatusBar>
#include <QString>
#include <QTextDocument>
#include <QtGui>
#include <QtWidgets>

#include "../configuration/configuration.h"

class QWidget;

static int breakLine(const QString &line, const int maxLength)
{
    // Check if line is a quote and avoid breaking it
    static const QRegularExpression quotedText(R"(^[[:space:]]*>)");
    if (quotedText.match(line).hasMatch()) {
        return -1;
    }

    // Break line into ANSI and text components
    QStringList textList, ansiList;
    const auto extract_parts = [&textList, &ansiList](const QString &line) {
        int textIndex = 0, ansiIndex = 0;
        static const QRegularExpression ansiRx(R"(\x1b\[(?:\d+;)*\d+m)");
        QRegularExpressionMatchIterator it = ansiRx.globalMatch(line);
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            ansiIndex = match.capturedStart(0);
            textList << line.mid(textIndex, ansiIndex - textIndex);
            textIndex = match.capturedEnd(0);
            ansiList << line.mid(ansiIndex, textIndex - ansiIndex);
        }
        if (textIndex < line.length()) {
            textList << line.mid(textIndex);
        }
    };
    extract_parts(line);

    // Find break point in components for a given length
    const auto extract_break_pos = [&textList, &ansiList](const int maxLength) {
        int textPos = 0;
        int ansiAddition = 0;
        QStringListIterator textIterator(textList), ansiIterator(ansiList);
        while (textIterator.hasNext()) {
            textPos += textIterator.next().length();
            if (textPos > maxLength)
                return maxLength + ansiAddition;
            if (ansiIterator.hasNext())
                ansiAddition += ansiIterator.next().length();
        }
        return -1;
    };
    return extract_break_pos(maxLength);
}

class LineHighlighter final : public QSyntaxHighlighter
{
public:
    explicit LineHighlighter(int maxLength, QTextDocument *parent);
    virtual ~LineHighlighter() override;

    void highlightBlock(const QString &text) override
    {
        const int breakPos = breakLine(text, maxLength);
        if (breakPos > 0) {
            QTextCharFormat underlineFormat;
            underlineFormat.setFontUnderline(true);
            underlineFormat.setUnderlineStyle(QTextCharFormat::UnderlineStyle::WaveUnderline);
            underlineFormat.setUnderlineColor(Qt::red);
            auto length = text.length() - breakPos;
            setFormat(breakPos, length, underlineFormat);
        }
    }

private:
    const int maxLength;
};

LineHighlighter::LineHighlighter(int maxLength, QTextDocument *const parent)
    : QSyntaxHighlighter(parent)
    , maxLength(maxLength)
{}

LineHighlighter::~LineHighlighter() = default;

RemoteEditWidget::RemoteEditWidget(const bool editSession,
                                   const QString &title,
                                   const QString &body,
                                   QWidget *const parent)
    : QMainWindow(parent)
    , m_editSession(editSession)
    , m_title(title)
    , m_body(body)
{
    setWindowFlags(windowFlags() | Qt::WindowType::Widget);
    setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowTitle(m_title + " - MMapper " + (m_editSession ? "Editor" : "Viewer"));

    // Figure out what length we should limit ourselves to
    static const QRegularExpression titleLengthLimit79(
        R"(^(Enter new whois|Enter new description)$)");
    if (titleLengthLimit79.match(m_title).hasMatch()) {
        m_maxLength = 79;
    }

    // REVISIT: can this be called as an initializer?
    m_textEdit.reset(createTextEdit());

    // REVISIT: Restore geometry from config?
    setGeometry(QStyle::alignedRect(Qt::LeftToRight,
                                    Qt::AlignCenter,
                                    size(),
                                    qApp->desktop()->availableGeometry()));

    show();
    focusWidget();
    m_textEdit->setFocus(); // REVISIT: can this be done in the creation function?
}

QPlainTextEdit *RemoteEditWidget::createTextEdit()
{
    QFont font;
    font.fromString(getConfig().integratedClient.font);
    const QFontMetrics fm(font);
    const int x = fm.averageCharWidth() * (80 + 1);
    const int y = fm.lineSpacing() * (24 + 1);

    const auto pTextEdit = new QPlainTextEdit(this);
    pTextEdit->setFont(font);
    pTextEdit->setPlainText(m_body);
    pTextEdit->setReadOnly(!m_editSession);
    pTextEdit->setMinimumSize(QSize(x + contentsMargins().left() + contentsMargins().right(),
                                    y + contentsMargins().top() + contentsMargins().bottom()));
    pTextEdit->setLineWrapMode(QPlainTextEdit::LineWrapMode::NoWrap);
    pTextEdit->setSizeIncrement(fm.averageCharWidth(), fm.lineSpacing());
    pTextEdit->setTabStopWidth(fm.width(" ") * 8); // A tab is 8 spaces wide

    new LineHighlighter(m_maxLength, pTextEdit->document());

    setCentralWidget(pTextEdit);
    addStatusBar(pTextEdit);
    addFileMenu(menuBar(), pTextEdit);
    return pTextEdit;
}

void RemoteEditWidget::addFileMenu(QMenuBar *const menuBar, const QPlainTextEdit *pTextEdit)
{
    QMenu *const fileMenu = menuBar->addMenu(tr("&File"));
    if (m_editSession) {
        addSave(fileMenu);
    }
    addExit(fileMenu);
    addEditMenu(menuBar, pTextEdit);
}

void RemoteEditWidget::addEditMenu(QMenuBar *const menuBar, const QPlainTextEdit *pTextEdit)
{
    QMenu *const editMenu = menuBar->addMenu("&Edit");
    addCut(editMenu, pTextEdit);
    addCopy(editMenu, pTextEdit);
    addPaste(editMenu, pTextEdit);
    addJustify(editMenu);
}

void RemoteEditWidget::addSave(QMenu *const fileMenu)
{
    QAction *const saveAction = new QAction(QIcon::fromTheme("document-save",
                                                             QIcon(":/icons/save.png")),
                                            tr("&Submit"),
                                            this);
    saveAction->setShortcut(tr("Ctrl+S"));
    saveAction->setStatusTip(tr("Submit changes to MUME"));
    fileMenu->addAction(saveAction);
    connect(saveAction, &QAction::triggered, this, &RemoteEditWidget::finishEdit);
}

void RemoteEditWidget::addExit(QMenu *const fileMenu)
{
    QAction *const quitAction = new QAction(QIcon::fromTheme("window-close",
                                                             QIcon(":/icons/exit.png")),
                                            tr("E&xit"),
                                            this);
    quitAction->setShortcut(tr("Ctrl+Q"));
    quitAction->setStatusTip(tr("Cancel and do not submit changes to MUME"));
    fileMenu->addAction(quitAction);
    connect(quitAction, &QAction::triggered, this, &RemoteEditWidget::cancelEdit);
}

void RemoteEditWidget::addCut(QMenu *const editMenu, const QPlainTextEdit *pTextEdit)
{
    QAction *const cutAct = new QAction(QIcon::fromTheme("edit-cut", QIcon(":/icons/cut.png")),
                                        tr("Cu&t"),
                                        this);
    cutAct->setShortcut(tr("Ctrl+X"));
    cutAct->setStatusTip(tr("Cut the current selection's contents to the clipboard"));
    editMenu->addAction(cutAct);
    connect(cutAct, &QAction::triggered, pTextEdit, &QPlainTextEdit::cut);
    cutAct->setDisabled(!m_editSession);
}

void RemoteEditWidget::addCopy(QMenu *const editMenu, const QPlainTextEdit *pTextEdit)
{
    QAction *const copyAct = new QAction(QIcon::fromTheme("edit-copy", QIcon(":/icons/copy.png")),
                                         tr("&Copy"),
                                         this);
    copyAct->setShortcut(tr("Ctrl+C"));
    copyAct->setStatusTip(tr("Copy the current selection's contents to the clipboard"));
    editMenu->addAction(copyAct);
    connect(copyAct, &QAction::triggered, pTextEdit, &QPlainTextEdit::copy);
}

void RemoteEditWidget::addPaste(QMenu *const editMenu, const QPlainTextEdit *pTextEdit)
{
    QAction *const pasteAct = new QAction(QIcon::fromTheme("edit-paste", QIcon(":/icons/paste.png")),
                                          tr("&Paste"),
                                          this);
    pasteAct->setShortcut(tr("Ctrl+V"));
    pasteAct->setStatusTip(tr("Paste the clipboard's contents into the current selection"));
    editMenu->addAction(pasteAct);
    connect(pasteAct, &QAction::triggered, pTextEdit, &QPlainTextEdit::paste);
    pasteAct->setDisabled(!m_editSession);
}

void RemoteEditWidget::addJustify(QMenu *editMenu)
{
    QAction *const justifyAct = new QAction(tr("&Justify"), this);
    justifyAct->setShortcut(tr("Ctrl+J"));
    justifyAct->setStatusTip(tr("Justify text to 80 characters"));
    editMenu->addAction(justifyAct);
    connect(justifyAct, &QAction::triggered, this, &RemoteEditWidget::justifyText);
    justifyAct->setDisabled(!m_editSession);
}

void RemoteEditWidget::addStatusBar(const QPlainTextEdit *pTextEdit)
{
    QStatusBar *status = statusBar();
    status->showMessage(tr("Ready"));

    connect(pTextEdit,
            &QPlainTextEdit::cursorPositionChanged,
            this,
            &RemoteEditWidget::updateStatusBar);
    connect(pTextEdit, &QPlainTextEdit::selectionChanged, this, &RemoteEditWidget::updateStatusBar);
}

void RemoteEditWidget::updateStatusBar()
{
    static const QRegularExpression newlineRx(R"((?!\r)\n)");
    auto cursor = m_textEdit->textCursor();
    const int column = cursor.columnNumber() + 1;
    const int row = cursor.blockNumber() + 1;
    const QString selection = cursor.selection().toPlainText();
    const int selectionLength = selection.length();
    const int selectionLines = selection.count(newlineRx);
    const QString text = QString("Line %1, Column %2, Selection %3 | %4")
                             .arg(row)
                             .arg(column)
                             .arg(selectionLength)
                             .arg(selectionLines);
    statusBar()->showMessage(text);
}

void RemoteEditWidget::justifyText()
{
    const QString &old = m_textEdit->toPlainText();
    QString text;
    text.reserve(old.length());
    for (const QString &line : old.split('\n')) {
        if (line.length() <= m_maxLength) {
            text.append(line);

        } else {
            QString remainder = line;
            int breakPos = -1;
            while ((breakPos = breakLine(remainder, m_maxLength)) > 0) {
                text.append(remainder.mid(0, breakPos));
                text.append("\n");
                remainder = remainder.mid(breakPos);
            }
            text.append(remainder);
        }
        text.append("\n");
    }
    m_textEdit->setPlainText(text);
}

RemoteEditWidget::~RemoteEditWidget()
{
    qInfo() << "Destroyed RemoteEditWidget" << m_title;
}

QSize RemoteEditWidget::minimumSizeHint() const
{
    return QSize{100, 100};
}

QSize RemoteEditWidget::sizeHint() const
{
    return QSize{640, 480};
}

void RemoteEditWidget::closeEvent(QCloseEvent *event)
{
    if (m_editSession) {
        if (m_submitted || maybeCancel()) {
            event->accept();
        } else {
            event->ignore();
        }
    } else {
        cancelEdit();
        event->accept();
    }
}

bool RemoteEditWidget::maybeCancel()
{
    if (contentsChanged()) {
        const int ret = QMessageBox::warning(this,
                                             m_title,
                                             tr("You have edited the document.\n"
                                                "Do you want to cancel your changes?"),
                                             QMessageBox::Yes,
                                             QMessageBox::No | QMessageBox::Escape
                                                 | QMessageBox::Default);
        if (ret == QMessageBox::No) {
            return false;
        }
    }

    cancelEdit();
    return true;
}

bool RemoteEditWidget::contentsChanged() const
{
    const QString text = m_textEdit->toPlainText();
    return QString::compare(text, m_body, Qt::CaseSensitive) != 0;
}

void RemoteEditWidget::cancelEdit()
{
    m_submitted = true;
    emit cancel();
    close();
}

void RemoteEditWidget::finishEdit()
{
    m_submitted = true;
    emit save(m_textEdit->toPlainText());
    close();
}
