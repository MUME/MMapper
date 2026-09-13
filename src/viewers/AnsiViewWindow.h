#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "../global/macros.h"

#include <memory>
#include <string_view>

#include <QDialog>
#include <QString>

class QTextBrowser;

class NODISCARD AnsiViewWindow final : public QDialog
{
private:
    std::unique_ptr<QTextBrowser> m_view;

public:
    explicit AnsiViewWindow(const QString &program, const QString &title, std::string_view message);
    ~AnsiViewWindow() final;
};

// Yes, this interface is a bit silly to mix Qt and std types,
// but the program/title are for the UI,
// while the body is interpreted to format text into a document.
//
// Returns a QDialog rather than a concrete AnsiViewWindow so callers (see
// mpi/remoteeditwidget.h's m_preview and viewers/LaunchAsyncViewer.h) never
// need AnsiViewWindow to be a complete type; the object returned is always
// an AnsiViewWindow, which is-a QDialog.
NODISCARD std::unique_ptr<QDialog> makeAnsiViewWindow(const QString &program,
                                                      const QString &title,
                                                      std::string_view body);
