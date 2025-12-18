// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "clientconfigpage.h"

#include "../configuration/HotkeyManager.h"
#include "../configuration/configuration.h"
#include "../global/macros.h"
#include "ui_clientconfigpage.h"

#include <QFileDialog>
#include <QMessageBox>

ClientConfigPage::ClientConfigPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ClientConfigPage)
{
    ui->setupUi(this);

    connect(ui->exportButton, &QPushButton::clicked, this, &ClientConfigPage::slot_onExport);
    connect(ui->importButton, &QPushButton::clicked, this, &ClientConfigPage::slot_onImport);
}

ClientConfigPage::~ClientConfigPage()
{
    delete ui;
}

void ClientConfigPage::slot_loadConfig()
{
    // Nothing to load - checkboxes maintain their own state
}

void ClientConfigPage::slot_onExport()
{
    // Build content using _hotkey command syntax
    QString content;
    {
        const auto &hotkeyManager = getConfig().hotkeyManager;
        const QStringList keyNames = hotkeyManager.getAllKeyNames();
        for (const QString &keyName : keyNames) {
            QString command = hotkeyManager.getCommand(keyName);
            if (!command.isEmpty()) {
                content += QString("_hotkey %1 %2\n").arg(keyName, command);
            }
        }
    }

    // Future: add more sections here (aliases, triggers, etc.)

    QFileDialog::saveFileContent(content.toUtf8(), "mmapper-config.txt");
}

void ClientConfigPage::slot_onImport()
{
    const auto nameFilter = tr("Text Files (*.txt);;All Files (*)");

    // Callback to process the imported file content
    const auto processImportedFile = [this](const QString &fileName, const QByteArray &fileContent) {
        if (fileName.isEmpty()) {
            return; // User cancelled
        }

        int hotkeyCount = 0;

        // Clear existing hotkeys and import new ones
        setConfig().hotkeyManager.clear();

        // Parse _hotkey commands line by line
        const QString content = QString::fromUtf8(fileContent);
        const QStringList lines = content.split('\n', Qt::SkipEmptyParts);
        for (const QString &line : lines) {
            QString trimmed = line.trimmed();
            if (trimmed.startsWith("_hotkey ")) {
                // Parse: _hotkey KEY command...
                QString rest = trimmed.mid(8); // Skip "_hotkey "
                qsizetype spaceIdx = rest.indexOf(' ');
                if (spaceIdx > 0) {
                    QString keyName = rest.left(spaceIdx);
                    QString command = rest.mid(spaceIdx + 1);
                    if (!keyName.isEmpty() && !command.isEmpty()) {
                        setConfig().hotkeyManager.setHotkey(keyName, command);
                        hotkeyCount++;
                    }
                }
            }
        }

        // Future: import more sections here (aliases, triggers, etc.)

        if (hotkeyCount > 0) {
            QMessageBox::information(this,
                                     tr("Import Successful"),
                                     tr("Configuration imported from:\n%1\n\n%2 hotkeys imported.")
                                         .arg(fileName)
                                         .arg(hotkeyCount));
        } else {
            QMessageBox::warning(this,
                                 tr("Import Warning"),
                                 tr("No hotkeys found in file.\n\n"
                                    "Expected format: _hotkey KEY command"));
        }
    };

    QFileDialog::getOpenFileContent(nameFilter, processImportedFile);
}
