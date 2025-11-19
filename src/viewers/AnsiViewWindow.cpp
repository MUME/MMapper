// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "AnsiViewWindow.h"

#include "../client/displaywidget.h"
#include "../global/macros.h"
#include "../global/utils.h"
#include "../global/window_utils.h"

#include <tuple>

#include <QDialog>
#include <QStyle>
#include <QTextBrowser>
#include <QVBoxLayout>

AnsiViewWindow::AnsiViewWindow(const QString &program,
                               const QString &title,
                               const std::string_view message)
    : m_view{std::make_unique<QTextBrowser>(this)}
{
    setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    mmqt::setWindowTitle2(*this, program, title);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    auto &view = deref(m_view);
    setAnsiText(&view, message);
    view.setOpenExternalLinks(true);
    view.setTextInteractionFlags(Qt::TextBrowserInteraction);

    // REVISIT: Restore geometry from config?
    setGeometry(QStyle::alignedRect(Qt::LeftToRight,
                                    Qt::AlignCenter,
                                    size(),
                                    qApp->primaryScreen()->availableGeometry()));

    show();
    raise();
    activateWindow();
    mainLayout->addWidget(&view, 0);
    view.setFocus(); // REVISIT: can this be done in the creation function?
}

AnsiViewWindow::~AnsiViewWindow() = default;

std::unique_ptr<AnsiViewWindow> makeAnsiViewWindow(const QString &program,
                                                   const QString &title,
                                                   std::string_view body)
{
    return std::make_unique<AnsiViewWindow>(program, title, body);
}
