// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "../mapdata/mapdata.h"
#include "mainwindow.h"

#include <memory>

#include <QMessageBox>
#include <QStatusBar>

NODISCARD static QStringList getSaveFileNames(std::unique_ptr<QFileDialog> &&ptr)
{
    if (const auto pSaveDialog = ptr.get()) {
        if (pSaveDialog->exec() == QDialog::Accepted)
            return pSaveDialog->selectedFiles();
        return QStringList{};
    }
    throw NullPointerException();
}

bool MainWindow::maybeSave()
{
    if (!m_mapData->dataChanged())
        return true;

    const int ret = QMessageBox::warning(this,
                                         tr("mmapper"),
                                         tr("The document has been modified.\n"
                                            "Do you want to save your changes?"),
                                         QMessageBox::Yes | QMessageBox::Default,
                                         QMessageBox::No,
                                         QMessageBox::Cancel | QMessageBox::Escape);

    if (ret == QMessageBox::Yes) {
        return slot_save();
    }

    // REVISIT: is it a bug if this returns true? (Shouldn't this always be false?)
    return ret != QMessageBox::Cancel;
}

std::unique_ptr<QFileDialog> MainWindow::createDefaultSaveDialog()
{
    const auto &path = getConfig().autoLoad.lastMapDirectory;
    QDir dir;
    if (dir.mkpath(path))
        dir.setPath(path);
    else
        dir.setPath(QDir::homePath());
    auto save = std::make_unique<QFileDialog>(this, "Choose map file name ...");
    save->setFileMode(QFileDialog::AnyFile);
    save->setDirectory(dir);
    save->setNameFilter("MMapper Maps (*.mm2)");
    save->setDefaultSuffix("mm2");
    save->setAcceptMode(QFileDialog::AcceptSave);
    return save;
}

bool MainWindow::slot_save()
{
    if (m_mapData->getFileName().isEmpty() || m_mapData->isFileReadOnly()) {
        return slot_saveAs();
    }
    return saveFile(m_mapData->getFileName(), SaveModeEnum::FULL, SaveFormatEnum::MM2);
}

bool MainWindow::slot_saveAs()
{
    const auto makeSaveDialog = [this]() {
        auto saveDialog = createDefaultSaveDialog();
        QFileInfo currentFile(m_mapData->getFileName());
        if (currentFile.exists()) {
            QString suggestedFileName = currentFile.fileName()
                                            .replace(QRegularExpression(R"(\.mm2$)"), "-copy.mm2")
                                            .replace(QRegularExpression(R"(\.xml$)"), "-import.mm2");
            saveDialog->selectFile(suggestedFileName);
        }
        return saveDialog;
    };
    const QStringList fileNames = getSaveFileNames(makeSaveDialog());
    if (fileNames.isEmpty()) {
        statusBar()->showMessage(tr("No filename provided"), 2000);
        return false;
    }

    QFileInfo file(fileNames[0]);
    bool success = saveFile(file.absoluteFilePath(), SaveModeEnum::FULL, SaveFormatEnum::MM2);
    if (success) {
        // Update last directory
        auto &config = setConfig().autoLoad;
        config.lastMapDirectory = file.absoluteDir().absolutePath();

        // Check if this should be the new autoload map
        QMessageBox dlg(QMessageBox::Question,
                        "Autoload Map?",
                        "Autoload this map when MMapper starts?",
                        QMessageBox::StandardButtons{QMessageBox::Yes | QMessageBox::No},
                        this);
        if (dlg.exec() == QMessageBox::Yes) {
            config.autoLoadMap = true;
            config.fileName = file.absoluteFilePath();
        }
    }
    return success;
}

bool MainWindow::slot_exportBaseMap()
{
    const auto makeSaveDialog = [this]() {
        auto saveDialog = createDefaultSaveDialog();
        QFileInfo currentFile(m_mapData->getFileName());
        if (currentFile.exists()) {
            saveDialog->selectFile(
                currentFile.fileName().replace(QRegularExpression(R"(\.mm2$)"), "-base.mm2"));
        }
        return saveDialog;
    };

    const auto fileNames = getSaveFileNames(makeSaveDialog());
    if (fileNames.isEmpty()) {
        statusBar()->showMessage(tr("No filename provided"), 2000);
        return false;
    }

    return saveFile(fileNames[0], SaveModeEnum::BASEMAP, SaveFormatEnum::MM2);
}

bool MainWindow::slot_exportMm2xmlMap()
{
    const auto makeSaveDialog = [this]() {
        // FIXME: code duplication
        auto save = std::make_unique<QFileDialog>(this,
                                                  "Choose map file name ...",
                                                  QDir::current().absolutePath());
        save->setFileMode(QFileDialog::AnyFile);
        save->setDirectory(QDir(getConfig().autoLoad.lastMapDirectory));
        save->setNameFilter("MM2XML Maps (*.mm2xml)");
        save->setDefaultSuffix("mm2xml");
        save->setAcceptMode(QFileDialog::AcceptSave);
        save->selectFile(QFileInfo(m_mapData->getFileName())
                             .fileName()
                             .replace(QRegularExpression(R"(\.mm2$)"), ".mm2xml"));

        return save;
    };

    const auto fileNames = getSaveFileNames(makeSaveDialog());
    if (fileNames.isEmpty()) {
        statusBar()->showMessage(tr("No filename provided"), 2000);
        return false;
    }
    return saveFile(fileNames[0], SaveModeEnum::FULL, SaveFormatEnum::MM2XML);
}

bool MainWindow::slot_exportWebMap()
{
    const auto makeSaveDialog = [this]() {
        // FIXME: code duplication
        auto save = std::make_unique<QFileDialog>(this,
                                                  "Choose map file name ...",
                                                  QDir::current().absolutePath());
        save->setFileMode(QFileDialog::Directory);
        save->setOption(QFileDialog::ShowDirsOnly, true);
        save->setAcceptMode(QFileDialog::AcceptSave);
        save->setDirectory(QDir(getConfig().autoLoad.lastMapDirectory));
        return save;
    };

    const QStringList fileNames = getSaveFileNames(makeSaveDialog());
    if (fileNames.isEmpty()) {
        statusBar()->showMessage(tr("No filename provided"), 2000);
        return false;
    }

    return saveFile(fileNames[0], SaveModeEnum::BASEMAP, SaveFormatEnum::WEB);
}

bool MainWindow::slot_exportMmpMap()
{
    const auto makeSaveDialog = [this]() {
        // FIXME: code duplication
        auto save = std::make_unique<QFileDialog>(this,
                                                  "Choose map file name ...",
                                                  QDir::current().absolutePath());
        save->setFileMode(QFileDialog::AnyFile);
        save->setDirectory(QDir(getConfig().autoLoad.lastMapDirectory));
        save->setNameFilter("MMP Maps (*.xml)");
        save->setDefaultSuffix("xml");
        save->setAcceptMode(QFileDialog::AcceptSave);
        save->selectFile(QFileInfo(m_mapData->getFileName())
                             .fileName()
                             .replace(QRegularExpression(R"(\.mm2$)"), "-mmp.xml"));

        return save;
    };

    const auto fileNames = getSaveFileNames(makeSaveDialog());
    if (fileNames.isEmpty()) {
        statusBar()->showMessage(tr("No filename provided"), 2000);
        return false;
    }
    return saveFile(fileNames[0], SaveModeEnum::FULL, SaveFormatEnum::MMP);
}
