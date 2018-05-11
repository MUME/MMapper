/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor),
**            Nils Schimmelmann <nschimme@gmail.com> (Jahara)
**
** This file is part of the MMapper project.
** Maintained by Nils Schimmelmann <nschimme@gmail.com>
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the:
** Free Software Foundation, Inc.
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
************************************************************************/

#include "generalpage.h"
#include "configuration.h"

#include <QFileDialog>
#include <QColorDialog>

GeneralPage::GeneralPage(QWidget *parent)
    : QWidget(parent)
{
    setupUi(this);
    connect( remoteName, SIGNAL( textChanged(const QString &) ), this,
             SLOT( remoteNameTextChanged(const QString &) )  );
    connect( remotePort, SIGNAL( valueChanged(int) ), this, SLOT( remotePortValueChanged(int) )  );
    connect( localPort, SIGNAL( valueChanged(int) ), this, SLOT( localPortValueChanged(int) )  );
    connect( tlsEncryptionCheckBox, SIGNAL(stateChanged(int)),
             SLOT(tlsEncryptionCheckBoxStateChanged(int)));

    connect( changeColor, SIGNAL(clicked()), SLOT(changeColorClicked()));
    connect( antialiasingSamplesComboBox, SIGNAL( currentTextChanged(const QString &) ), this,
             SLOT( antialiasingSamplesTextChanged(const QString &) )  );
    connect( trilinearFilteringCheckBox, SIGNAL(stateChanged(int)),
             SLOT(trilinearFilteringStateChanged(int)));
    connect( softwareOpenGLCheckBox, SIGNAL(stateChanged(int)),
             SLOT(softwareOpenGLStateChanged(int)));

    connect ( emulatedExitsCheckBox, SIGNAL(stateChanged(int)), SLOT(emulatedExitsStateChanged(int)));
    connect ( showHiddenExitFlagsCheckBox, SIGNAL(stateChanged(int)),
              SLOT(showHiddenExitFlagsStateChanged(int)));
    connect ( showNotesCheckBox, SIGNAL(stateChanged(int)), SLOT(showNotesStateChanged(int)));

    connect ( updated, SIGNAL(stateChanged(int)), SLOT(updatedStateChanged(int)));
    connect ( drawNotMappedExits, SIGNAL(stateChanged(int)), SLOT(drawNotMappedExitsStateChanged(int)));
    connect ( drawNoMatchExits, SIGNAL(stateChanged(int)), SLOT(drawNoMatchExitsStateChanged(int)));
    connect ( drawDoorNames, SIGNAL(stateChanged(int)), SLOT(drawDoorNamesStateChanged(int)));
    connect ( drawUpperLayersTextured, SIGNAL(stateChanged(int)),
              SLOT(drawUpperLayersTexturedStateChanged(int)));

    connect( autoLoadFileName, SIGNAL( textChanged(const QString &) ), this,
             SLOT( autoLoadFileNameTextChanged(const QString &) )  );
    connect( autoLoadCheck, SIGNAL(stateChanged(int)), SLOT(autoLoadCheckStateChanged(int)));

    connect( sellectWorldFileButton, SIGNAL(clicked()), this, SLOT(selectWorldFileButtonClicked()));

    connect( displayMumeClockCheckBox, SIGNAL(stateChanged(int)),
             SLOT(displayMumeClockStateChanged(int)));

    remoteName->setText( Config().m_remoteServerName );
    remotePort->setValue( Config().m_remotePort );
    localPort->setValue( Config().m_localPort );
    tlsEncryptionCheckBox->setChecked( Config().m_tlsEncryption );

    QPixmap bgPix(16, 16);
    bgPix.fill(Config().m_backgroundColor);
    changeColor->setIcon(QIcon(bgPix));
    int index = antialiasingSamplesComboBox->findText(QString::number(Config().m_antialiasingSamples));
    if (index < 0) index = 0;
    antialiasingSamplesComboBox->setCurrentIndex(index);
    trilinearFilteringCheckBox->setChecked(Config().m_trilinearFiltering);
    softwareOpenGLCheckBox->setChecked(Config().m_softwareOpenGL);

    emulatedExitsCheckBox->setChecked( Config().m_emulatedExits );
    showHiddenExitFlagsCheckBox->setChecked( Config().m_showHiddenExitFlags );
    showNotesCheckBox->setChecked( Config().m_showNotes );

    updated->setChecked( Config().m_showUpdated );
    drawNotMappedExits->setChecked( Config().m_drawNotMappedExits );
    drawNoMatchExits->setChecked( Config().m_drawNoMatchExits );
    drawUpperLayersTextured->setChecked( Config().m_drawUpperLayersTextured );
    drawDoorNames->setChecked( Config().m_drawDoorNames );

    autoLoadCheck->setChecked( Config().m_autoLoadWorld );
    autoLoadFileName->setText( Config().m_autoLoadFileName );

    displayMumeClockCheckBox->setChecked(Config().m_displayMumeClock);
}

void GeneralPage::changeColorClicked()
{
    const QColor newColor = QColorDialog::getColor(Config().m_backgroundColor, this);
    if (newColor.isValid() && newColor != Config().m_backgroundColor) {
        QPixmap bgPix(16, 16);
        bgPix.fill(newColor);
        changeColor->setIcon(QIcon(bgPix));
        Config().m_backgroundColor = newColor;
    }
}

void GeneralPage::antialiasingSamplesTextChanged(const QString &)
{
    Config().m_antialiasingSamples = antialiasingSamplesComboBox->currentText().toInt();
}

void GeneralPage::trilinearFilteringStateChanged(int)
{
    Config().m_trilinearFiltering = trilinearFilteringCheckBox->isChecked();
}

void GeneralPage::softwareOpenGLStateChanged(int)
{
    Config().m_softwareOpenGL = softwareOpenGLCheckBox->isChecked();
}

void GeneralPage::selectWorldFileButtonClicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Choose map file ...", "",
                                                    "MMapper2 (*.mm2);;MMapper (*.map)");
    if (!fileName.isEmpty()) {
        autoLoadFileName->setText( fileName );
        Config().m_autoLoadFileName = fileName;
        autoLoadCheck->setChecked(true);
        Config().m_autoLoadWorld = true;
    }
}

void GeneralPage::remoteNameTextChanged(const QString &)
{
    Config().m_remoteServerName = remoteName->text();
}

void GeneralPage::remotePortValueChanged(int)
{
    Config().m_remotePort = remotePort->value();
}

void GeneralPage::localPortValueChanged(int)
{
    Config().m_localPort = localPort->value();
}

void GeneralPage::tlsEncryptionCheckBoxStateChanged(int)
{
    Config().m_tlsEncryption = tlsEncryptionCheckBox->isChecked();
}

void GeneralPage::emulatedExitsStateChanged(int)
{
    Config().m_emulatedExits = emulatedExitsCheckBox->isChecked();
}

void GeneralPage::showHiddenExitFlagsStateChanged(int)
{
    Config().m_showHiddenExitFlags = showHiddenExitFlagsCheckBox->isChecked();
}

void GeneralPage::showNotesStateChanged(int)
{
    Config().m_showNotes = showNotesCheckBox->isChecked();
}

void GeneralPage::updatedStateChanged(int)
{
    Config().m_showUpdated = updated->isChecked();
}

void GeneralPage::drawNotMappedExitsStateChanged(int)
{
    Config().m_drawNotMappedExits = drawNotMappedExits->isChecked();
}

void GeneralPage::drawNoMatchExitsStateChanged(int)
{
    Config().m_drawNoMatchExits = drawNoMatchExits->isChecked();
}

void GeneralPage::drawDoorNamesStateChanged(int)
{
    Config().m_drawDoorNames = drawDoorNames->isChecked();
}

void GeneralPage::drawUpperLayersTexturedStateChanged(int)
{
    Config().m_drawUpperLayersTextured = drawUpperLayersTextured->isChecked();
}

void GeneralPage::autoLoadFileNameTextChanged(const QString &)
{
    Config().m_autoLoadFileName = autoLoadFileName->text();
}

void GeneralPage::autoLoadCheckStateChanged(int)
{
    Config().m_autoLoadWorld = autoLoadCheck->isChecked();
}

void GeneralPage::displayMumeClockStateChanged(int)
{
    Config().m_displayMumeClock = displayMumeClockCheckBox->isChecked();
}
