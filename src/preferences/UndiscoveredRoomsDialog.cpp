// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "UndiscoveredRoomsDialog.h"

#include "../configuration/configuration.h"

#include <QCheckBox>
#include <QCoreApplication>
#include <QMessageBox>

QMessageBox &showUndiscoveredRoomsDialog(QWidget *const parent)
{
    const auto heading = QCoreApplication::translate("UndiscoveredRoomsDialog",
                                                     "Loading undiscovered rooms.");
    auto *const message = new QMessageBox(QMessageBox::Information,
                                          heading,
                                          heading,
                                          QMessageBox::Ok | QMessageBox::Cancel,
                                          parent);
    // macOS ignores the window title: put the heading in the message body as well.
    message->setInformativeText(QCoreApplication::translate(
        "UndiscoveredRoomsDialog",
        "This may take about a minute in the background the first time you use it for each character."));
    message->setDefaultButton(QMessageBox::Ok);
    auto *const dontShowAgain = new QCheckBox(QCoreApplication::translate("UndiscoveredRoomsDialog",
                                                                          "Don't show again."),
                                              message);
    message->setCheckBox(dontShowAgain);
    QObject::connect(message, &QDialog::finished, message, [dontShowAgain](const int answer) {
        if (answer == QMessageBox::Ok && dontShowAgain->isChecked()) {
            setConfig().canvas.showUndiscoveredRoomsWarning.set(false);
        }
    });
    // Match MMapper's non-blocking message-box helpers, but configure everything
    // before open() so native dialogs receive their buttons, checkbox and text.
    message->setAttribute(Qt::WA_DeleteOnClose);
    message->open();
    return *message;
}
