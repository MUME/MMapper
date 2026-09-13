// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors
// Author: Mike Repass <mike.repass@gmail.com> (Taryn)

#include "adventurewidget.h"

#include "../configuration/configuration.h"
#include "../global/Consts.h"
#include "../global/window_utils.h"

#include <memory>

#include <QtCore>
#include <QtWidgets>

AdventureWidget::AdventureWidget(AdventureTracker &at, QWidget *const parent)
    : QWidget{parent}
{
    m_model = new AdventureLogModel(at, this);

    m_textEdit = new QTextEdit(this);
    m_textEdit->setReadOnly(true);
    m_textEdit->setUndoRedoEnabled(false);
    m_textEdit->setDocumentTitle("Adventure Panel Text");
    m_textEdit->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_textEdit->setTabChangesFocus(false);

    const auto &settings = getConfig().integratedClient;

    auto *const document = m_textEdit->document();
    QTextFrameFormat frameFormat = document->rootFrame()->frameFormat();
    frameFormat.setBackground(settings.backgroundColor);
    document->rootFrame()->setFrameFormat(frameFormat);

    QTextCharFormat blockCharFormat = QTextCursor(document).blockCharFormat();
    blockCharFormat.setForeground(settings.foregroundColor);
    {
        QFont font;
        font.fromString(settings.font); // need fromString() to extract PointSize
        blockCharFormat.setFont(font);
    }
    QTextCursor(document).setBlockCharFormat(blockCharFormat);

    auto *const layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignTop);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_textEdit);

    m_clearContentAction = new QAction("Clear Content", this);
    connect(m_clearContentAction, &QAction::triggered, m_model, &AdventureLogModel::clear);

    m_textEdit->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_textEdit,
            &QTextEdit::customContextMenuRequested,
            this,
            &AdventureWidget::slot_contextMenuRequested);

    connect(m_model, &QAbstractItemModel::rowsInserted, this, &AdventureWidget::slot_rowsInserted);
    connect(m_model, &QAbstractItemModel::rowsRemoved, this, &AdventureWidget::slot_rowsRemoved);

    // The model already holds its default welcome line.
    if (const int rows = m_model->rowCount(); rows > 0) {
        appendRows(0, rows - 1);
    }
}

void AdventureWidget::slot_rowsInserted(const QModelIndex &parent, const int first, const int last)
{
    if (parent.isValid()) {
        return;
    }
    appendRows(first, last);
}

void AdventureWidget::slot_rowsRemoved(const QModelIndex &parent, const int first, const int last)
{
    if (parent.isValid()) {
        return;
    }
    removeRows(first, last);
}

void AdventureWidget::appendRows(const int first, const int last)
{
    // Rows are only ever appended (see AdventureLogModel::addAdventureUpdate()),
    // so the new blocks go at the end of the document.
    assert(first == m_textEdit->document()->blockCount() - 1);

    QTextCursor cursor(m_textEdit->document());
    cursor.movePosition(QTextCursor::End);
    for (int row = first; row <= last; ++row) {
        cursor.insertText(m_model->data(m_model->index(row)).toString());
        cursor.insertText(QString(char_consts::C_NEWLINE));
    }

    auto *const scrollBar = m_textEdit->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}

void AdventureWidget::removeRows(const int first, const int last)
{
    // One document block per model row (plus the trailing empty block after
    // the final newline), so removing rows [first, last] is removing the
    // same range of blocks.
    QTextCursor cursor(m_textEdit->document());
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextBlock, QTextCursor::MoveAnchor, first);
    cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor, last - first + 1);
    cursor.removeSelectedText();
}

void AdventureWidget::slot_contextMenuRequested(const QPoint &pos)
{
    std::unique_ptr<QMenu> contextMenu{m_textEdit->createStandardContextMenu()};
    contextMenu->addSeparator();
    contextMenu->addAction(m_clearContentAction);
    mmqt::popupMenu(std::move(contextMenu), m_textEdit->mapToGlobal(pos));
}
