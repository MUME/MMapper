// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "../configuration/configuration.h"
#include "../mapdata/mapdata.h"
#include "../mapstorage/MapDestination.h"
#include "mainwindow.h"

#include <memory>

#include <QBuffer>
#include <QFileDialog>
#include <QMessageBox>

namespace { // anonymous

NODISCARD QStringList getSaveFileNames(std::unique_ptr<QFileDialog> &&ptr)
{
    if (const auto &pSaveDialog = ptr.get()) {
        if (pSaveDialog->exec() == QDialog::Accepted) {
            return pSaveDialog->selectedFiles();
        }
        return QStringList{};
    }
    throw NullPointerException();
}

namespace mwss_detail {

NODISCARD QDir getLastMapDir()
{
    const QString &path = getConfig().autoLoad.lastMapDirectory;
    QDir dir;
    if (dir.mkpath(path)) {
        dir.setPath(path);
    } else {
        dir.setPath(QDir::homePath());
    }
    return dir;
}

NODISCARD std::unique_ptr<QFileDialog> createCommonSaveDialog(MainWindow &mainWindow,
                                                              const QString &suggestedName)
{
    auto save = std::make_unique<QFileDialog>(&mainWindow, "Choose map file name ...");
    save->setAcceptMode(QFileDialog::AcceptSave);
    save->setDirectory(getLastMapDir());
    save->selectFile(suggestedName);
    return save;
}

NODISCARD std::unique_ptr<QFileDialog> createDirectorySaveDialog(MainWindow &mainWindow)
{
    auto save = createCommonSaveDialog(mainWindow, "");
    save->setFileMode(QFileDialog::Directory);
    save->setOption(QFileDialog::ShowDirsOnly, true);
    return save;
}

NODISCARD std::unique_ptr<QFileDialog> createFileSaveDialog(MainWindow &mainWindow,
                                                            const QString &nameFilter,
                                                            const QString &defaultSuffix,
                                                            const QString &suggestedName)
{
    auto save = createCommonSaveDialog(mainWindow, suggestedName);
    save->setFileMode(QFileDialog::AnyFile);
    save->setNameFilter(nameFilter);
    save->setDefaultSuffix(defaultSuffix);
    save->selectFile(suggestedName);
    return save;
}

NODISCARD std::unique_ptr<QFileDialog> createDefaultSaveDialog(MainWindow &mainWindow,
                                                               const QString &suggestedName)
{
    auto save = mwss_detail::createFileSaveDialog(mainWindow,
                                                  "MMapper maps (*.mm2)",
                                                  "mm2",
                                                  suggestedName);
    return save;
}

} // namespace mwss_detail
} // namespace

bool MainWindow::maybeSave()
{
    auto &mapData = deref(m_mapData);
    if (!mapData.dataChanged()) {
        return true;
    }

    const QString changes = mmqt::toQStringUtf8(mapData.describeChanges());

    QMessageBox dlg(this);
    dlg.setIcon(QMessageBox::Warning);
    dlg.setWindowTitle(tr("mmapper"));
    dlg.setText(tr("The current map has been modified:\n\n") + changes
                + tr("\nDo you want to save the changes?"));
    dlg.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    dlg.setDefaultButton(QMessageBox::Discard);
    dlg.setEscapeButton(QMessageBox::Cancel);
    const int ret = dlg.exec();
    if (ret == QMessageBox::Save) {
        return slot_save();
    }

    // REVISIT: is it a bug if this returns true? (Shouldn't this always be false?)
    return ret != QMessageBox::Cancel;
}

bool MainWindow::slot_save()
{
    if (m_mapData->getFileName().isEmpty() || m_mapData->isFileReadOnly()) {
        return slot_saveAs();
    }
    return saveFile(m_mapData->getFileName(), ::SaveModeEnum::FULL, ::SaveFormatEnum::MM2);
}

bool MainWindow::slot_saveAs()
{
    if (!tryStartNewAsync()) {
        return false;
    }

    QString suggestedName = m_mapData->getFileName();
    const QFileInfo currentFile(suggestedName);
    if (currentFile.exists()) {
        suggestedName = (currentFile.suffix().contains("xml")
                             ? currentFile.baseName().append("-import.mm2")
                             : currentFile.baseName().append("-copy.mm2"));
    }
    QString fileName = suggestedName;
    if constexpr (CURRENT_PLATFORM != PlatformEnum::Wasm) {
        const auto fileNames = getSaveFileNames(
            mwss_detail::createDefaultSaveDialog(*this, suggestedName));
        if (fileNames.isEmpty()) {
            showStatusShort(tr("No filename provided"));
            return false;
        }
        fileName = fileNames[0];
    }
    return saveFile(fileName, ::SaveModeEnum::FULL, ::SaveFormatEnum::MM2);
}

bool MainWindow::slot_exportBaseMap()
{
    const QString suggestedName = QFileInfo(m_mapData->getFileName()).baseName().append("-base.mm2");
    QString fileName = suggestedName;
    if constexpr (CURRENT_PLATFORM != PlatformEnum::Wasm) {
        const auto fileNames = getSaveFileNames(
            mwss_detail::createDefaultSaveDialog(*this, suggestedName));
        if (fileNames.isEmpty()) {
            showStatusShort(tr("No filename provided"));
            return false;
        }
        fileName = fileNames[0];
    }
    return saveFile(fileName, ::SaveModeEnum::BASEMAP, ::SaveFormatEnum::MM2);
}

bool MainWindow::slot_exportMm2xmlMap()
{
    const QString suggestedName = QFileInfo(m_mapData->getFileName()).baseName().append(".xml");
    QString fileName = suggestedName;
    if constexpr (CURRENT_PLATFORM != PlatformEnum::Wasm) {
        const auto fileNames = getSaveFileNames(
            mwss_detail::createFileSaveDialog(*this,
                                              "MMapper2 XML maps (*.xml)",
                                              "xml",
                                              suggestedName));
        if (fileNames.isEmpty()) {
            showStatusShort(tr("No filename provided"));
            return false;
        }
        fileName = fileNames[0];
    }
    return saveFile(fileName, ::SaveModeEnum::FULL, ::SaveFormatEnum::MM2XML);
}

bool MainWindow::slot_exportWebMap()
{
    if constexpr (CURRENT_PLATFORM == PlatformEnum::Wasm) {
        return false;
    }

    const QStringList fileNames = getSaveFileNames(mwss_detail::createDirectorySaveDialog(*this));
    if (fileNames.isEmpty()) {
        showStatusShort(tr("No directory name provided"));
        return false;
    }
    const QString dirName = fileNames[0];
    return saveFile(dirName, ::SaveModeEnum::BASEMAP, ::SaveFormatEnum::WEB);
}

bool MainWindow::slot_exportMmpMap()
{
    const QString suggestedName = QFileInfo(m_mapData->getFileName()).baseName().append("-mmp.xml");
    QString fileName = suggestedName;
    if constexpr (CURRENT_PLATFORM != PlatformEnum::Wasm) {
        const auto fileNames = getSaveFileNames(
            mwss_detail::createFileSaveDialog(*this, "MMP maps (*.xml)", "xml", suggestedName));
        if (fileNames.isEmpty()) {
            showStatusShort(tr("No filename provided"));
            return false;
        }
        fileName = fileNames[0];
    }

    return saveFile(fileName, ::SaveModeEnum::FULL, ::SaveFormatEnum::MMP);
}
