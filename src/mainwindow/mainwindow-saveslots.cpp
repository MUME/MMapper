// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "../configuration/configuration.h"
#include "../global/ConfigConsts-Computed.h"
#include "../mapdata/mapdata.h"
#include "../mapstorage/MapDestination.h"
#include "mainwindow.h"

#include <cassert>
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

NODISCARD std::unique_ptr<QMessageBox> createMaybeSaveDialog(MainWindow &mainWindow,
                                                             const QString &changes)
{
    auto dlg = std::make_unique<QMessageBox>(&mainWindow);
    dlg->setIcon(QMessageBox::Warning);
    dlg->setWindowTitle(MainWindow::tr("mmapper"));
    dlg->setText(MainWindow::tr("The current map has been modified:\n\n") + changes
                 + MainWindow::tr("\nDo you want to save the changes?"));
    dlg->setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    dlg->setDefaultButton(QMessageBox::Discard);
    dlg->setEscapeButton(QMessageBox::Cancel);
    return dlg;
}

} // namespace mwss_detail
} // namespace

bool MainWindow::handleMaybeSaveResult(const int result)
{
    switch (result) {
    case QMessageBox::Save:
        return slot_save();
    case QMessageBox::Discard:
        return true;
    default:
        // Cancel, Escape, or the window's close button.
        return false;
    }
}

void MainWindow::maybeSave(std::function<void()> onProceed)
{
    auto &mapData = deref(m_mapData);
    if (!mapData.dataChanged()) {
        onProceed();
        return;
    }

    const QString changes = mmqt::toQStringUtf8(mapData.describeChanges());
    // Ownership passes to Qt: the box deletes itself on close.
    auto *const dlg = mwss_detail::createMaybeSaveDialog(*this, changes).release();
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    connect(dlg,
            &QMessageBox::finished,
            this,
            [this, proceed = std::move(onProceed)](const int result) {
                if (handleMaybeSaveResult(result)) {
                    proceed();
                }
            });
    dlg->open();
}

bool MainWindow::maybeSaveBlocking()
{
    assert(CURRENT_PLATFORM != PlatformEnum::Wasm);
    auto &mapData = deref(m_mapData);
    if (!mapData.dataChanged()) {
        return true;
    }

    const QString changes = mmqt::toQStringUtf8(mapData.describeChanges());
    const auto dlg = mwss_detail::createMaybeSaveDialog(*this, changes);
    return handleMaybeSaveResult(dlg->exec());
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
