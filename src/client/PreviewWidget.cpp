// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "PreviewWidget.h"

#include "../configuration/configuration.h"
#include "displaywidget.h"

#include <QFont>
#include <QFontMetrics>
#include <QScrollBar>
#include <QTextEdit>

PreviewWidget::PreviewWidget(QWidget *parent)
    : QTextEdit(parent)
    , helper{*this}
{
    const auto &settings = getConfig().integratedClient;

    helper.init();

    // Set properties for non-interactive, non-scrollable display
    setReadOnly(true);
    setTextInteractionFlags(Qt::NoTextInteraction);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setFocusPolicy(Qt::NoFocus);
    setVisible(false);

    // Set maximum height based on configured lines and font metrics
    int lineCount = qMax(1, settings.linesOfPeekPreview);
    QFontMetrics fontMetrics(helper.format.font());
    int lineHeight = fontMetrics.height();
    setMaximumHeight(lineHeight * lineCount);
}

void PreviewWidget::displayText(const QString &textToShow)
{
    helper.displayText(textToShow);
    helper.limitScrollback(getConfig().integratedClient.linesOfPeekPreview);
    ensureCursorVisible();
}
