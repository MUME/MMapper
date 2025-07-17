// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "../mapdata/mapdata.h"
#include "mainwindow.h"

#include <memory>

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

NODISCARD std::unique_ptr<QFileDialog> createCommonSaveDialog(MainWindow &mainWindow)
{
    auto save = std::make_unique<QFileDialog>(&mainWindow, "Choose map file name ...");
    save->setAcceptMode(QFileDialog::AcceptSave);
    save->setDirectory(getLastMapDir());
    return save;
}

NODISCARD std::unique_ptr<QFileDialog> createDirectorySaveDialog(MainWindow &mainWindow)
{
    auto save = createCommonSaveDialog(mainWindow);
    save->setFileMode(QFileDialog::Directory);
    save->setOption(QFileDialog::ShowDirsOnly, true);
    return save;
}

NODISCARD std::unique_ptr<QFileDialog> createFileSaveDialog(MainWindow &mainWindow,
                                                            const QString &nameFilter,
                                                            const QString &defaultSuffix)
{
    auto save = createCommonSaveDialog(mainWindow);
    save->setFileMode(QFileDialog::AnyFile);
    save->setNameFilter(nameFilter);
    save->setDefaultSuffix(defaultSuffix);
    return save;
}

NODISCARD std::unique_ptr<QFileDialog> createDefaultSaveDialog(MainWindow &mainWindow)
{
    return mwss_detail::createFileSaveDialog(mainWindow, "MMapper maps (*.mm2)", "mm2");
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
    return saveFile(m_mapData->getFileName(), SaveModeEnum::FULL, SaveFormatEnum::MM2);
}

bool MainWindow::slot_saveAs()
{
    if (!tryStartNewAsync()) {
        return false;
    }

    const auto makeSaveDialog = [this]() {
        auto saveDialog = mwss_detail::createDefaultSaveDialog(*this);
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
        showStatusShort(tr("No filename provided"));
        return false;
    }

    QFileInfo file(fileNames[0]);
    return saveFile(file.absoluteFilePath(), SaveModeEnum::FULL, SaveFormatEnum::MM2);
}

bool MainWindow::slot_exportBaseMap()
{
    const auto makeSaveDialog = [this]() {
        auto saveDialog = mwss_detail::createDefaultSaveDialog(*this);
        QFileInfo currentFile(m_mapData->getFileName());
        if (currentFile.exists()) {
            saveDialog->selectFile(
                currentFile.fileName().replace(QRegularExpression(R"(\.mm2$)"), "-base.mm2"));
        }
        return saveDialog;
    };

    const auto fileNames = getSaveFileNames(makeSaveDialog());
    if (fileNames.isEmpty()) {
        showStatusShort(tr("No filename provided"));
        return false;
    }

    return saveFile(fileNames[0], SaveModeEnum::BASEMAP, SaveFormatEnum::MM2);
}

bool MainWindow::slot_exportMm2xmlMap()
{
    const auto makeSaveDialog = [this]() {
        const auto filename = QFileInfo(m_mapData->getFileName()).fileName();
        const auto suggestedFileName = QString(filename)
                                           .replace(QRegularExpression(R"(\.mm2$)"), ".xml")
                                           .replace(QRegularExpression(R"(\.mm2xml$)"), ".xml");

        // FIXME: Why is the filename set correctly sometimes but not others?
        qDebug() << "filename = " << filename;
        qDebug() << "suggestedFileName = " << suggestedFileName;

        auto save = mwss_detail::createFileSaveDialog(*this, "MMapper2 XML maps (*.xml)", "xml");
        save->selectFile(suggestedFileName);
        return save;
    };

    const auto fileNames = getSaveFileNames(makeSaveDialog());
    if (fileNames.isEmpty()) {
        showStatusShort(tr("No filename provided"));
        return false;
    }
    return saveFile(fileNames[0], SaveModeEnum::FULL, SaveFormatEnum::MM2XML);
}

bool MainWindow::slot_exportWebMap()
{
    const auto makeSaveDialog = [this]() { return mwss_detail::createDirectorySaveDialog(*this); };

    const QStringList fileNames = getSaveFileNames(makeSaveDialog());
    if (fileNames.isEmpty()) {
        showStatusShort(tr("No filename provided"));
        return false;
    }

    return saveFile(fileNames[0], SaveModeEnum::BASEMAP, SaveFormatEnum::WEB);
}

bool MainWindow::slot_exportMmpMap()
{
    const auto makeSaveDialog = [this]() {
        const auto suggestedFileName = QFileInfo(m_mapData->getFileName())
                                           .fileName()
                                           .replace(QRegularExpression(R"(\.mm2$)"), "-mmp.xml");

        auto save = mwss_detail::createFileSaveDialog(*this, "MMP maps (*.xml)", "xml");
        save->setAcceptMode(QFileDialog::AcceptSave);
        save->selectFile(suggestedFileName);

        return save;
    };

    const auto fileNames = getSaveFileNames(makeSaveDialog());
    if (fileNames.isEmpty()) {
        showStatusShort(tr("No filename provided"));
        return false;
    }
    return saveFile(fileNames[0], SaveModeEnum::FULL, SaveFormatEnum::MMP);
}
