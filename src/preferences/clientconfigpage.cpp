// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "clientconfigpage.h"

#include "../configuration/HotkeyManager.h"
#include "../configuration/configuration.h"
#include "../global/macros.h"
#include "ui_clientconfigpage.h"

#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>

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

QString ClientConfigPage::exportHotkeysToString() const
{
    // Add [Hotkeys] section header for .ini file format
    return "[Hotkeys]\n" + getConfig().hotkeyManager.exportToCliFormat();
}

void ClientConfigPage::slot_onExport()
{
    // Check if anything is selected
    if (!ui->exportHotkeysCheckBox->isChecked()) {
        QMessageBox::warning(this,
                             tr("Export Configuration"),
                             tr("Please select at least one section to export."));
        return;
    }

    // Build export content
    QString content;
    if (ui->exportHotkeysCheckBox->isChecked()) {
        content += exportHotkeysToString();
    }

    if constexpr (CURRENT_PLATFORM == PlatformEnum::Wasm) {
        // Use browser's native file download dialog
        QFileDialog::saveFileContent(content.toUtf8(), "mmapper-config.ini");
    } else {
        // Get file path using native dialog
        QString fileName = QFileDialog::getSaveFileName(this,
                                                        tr("Export Configuration"),
                                                        "mmapper-config.ini",
                                                        tr("INI Files (*.ini);;All Files (*)"));

        if (fileName.isEmpty()) {
            return;
        }

        // Write to file
        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::critical(this,
                                  tr("Export Failed"),
                                  tr("Could not open file for writing: %1").arg(file.errorString()));
            return;
        }

        QTextStream out(&file);
        out << content;
        file.close();

        QMessageBox::information(this,
                                 tr("Export Successful"),
                                 tr("Configuration exported to:\n%1").arg(fileName));
    }
}

bool ClientConfigPage::importFromString(const QString &content)
{
    // Extract content from [Hotkeys] section
    bool inHotkeysSection = false;
    bool foundHotkeysSection = false;
    QStringList hotkeyLines;

    const QStringList lines = content.split('\n');
    for (const QString &line : lines) {
        QString trimmedLine = line.trimmed();

        // Check for section headers
        if (trimmedLine.startsWith('[') && trimmedLine.endsWith(']')) {
            QString section = trimmedLine.mid(1, trimmedLine.length() - 2);
            if (section.compare("Hotkeys", Qt::CaseInsensitive) == 0) {
                inHotkeysSection = true;
                foundHotkeysSection = true;
            } else {
                inHotkeysSection = false;
            }
            continue;
        }

        // Collect lines from the Hotkeys section
        if (inHotkeysSection) {
            hotkeyLines.append(line);
        }
    }

    if (foundHotkeysSection) {
        setConfig().hotkeyManager.importFromCliFormat(hotkeyLines.join('\n'));
    }

    return foundHotkeysSection;
}

void ClientConfigPage::slot_onImport()
{
    const auto nameFilter = tr("INI Files (*.ini);;All Files (*)");

    // Callback to process the imported file content
    const auto processImportedFile = [this](const QString &fileName, const QByteArray &fileContent) {
        if (fileName.isEmpty()) {
            return; // User cancelled
        }

        const QString content = QString::fromUtf8(fileContent);

        // Import the content
        bool importedAnything = importFromString(content);

        if (importedAnything) {
            QMessageBox::information(this,
                                     tr("Import Successful"),
                                     tr("Configuration imported from:\n%1").arg(fileName));
        } else {
            QMessageBox::warning(this,
                                 tr("Import Warning"),
                                 tr("No recognized sections found in file.\n\n"
                                    "Expected sections: [Hotkeys]"));
        }
    };

    if constexpr (CURRENT_PLATFORM == PlatformEnum::Wasm) {
        // Use browser's native file upload dialog
        QFileDialog::getOpenFileContent(nameFilter, processImportedFile);
    } else {
        QString fileName = QFileDialog::getOpenFileName(this,
                                                        tr("Import Configuration"),
                                                        QString(),
                                                        nameFilter);

        if (fileName.isEmpty()) {
            return;
        }

        // Read file
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QMessageBox::critical(this,
                                  tr("Import Failed"),
                                  tr("Could not open file for reading: %1").arg(file.errorString()));
            return;
        }

        QTextStream in(&file);
        QString content = in.readAll();
        file.close();

        // Use the same processing logic
        processImportedFile(fileName, content.toUtf8());
    }
}
