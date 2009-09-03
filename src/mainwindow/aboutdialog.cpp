/************************************************************************
**
** Authors:   Kalev Lember <kalev@smartlink.ee>
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

#include "aboutdialog.h"

#include <QFile>
#include <QTextStream>

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowIcon(QIcon(":/icons/m.png"));
    setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    aboutTabLayout->setAlignment(Qt::AlignHCenter);

    /* About tab */
    pixmapLabel->setPixmap(QPixmap(":/pixmaps/splash20.png"));
    pixmapLabel->setFixedSize(QSize(pixmapLabel->pixmap()->width(), pixmapLabel->pixmap()->height()));
    pixmapLabel->setAlignment(Qt::AlignCenter);

    aboutText->setAlignment(Qt::AlignCenter);
    aboutText->setTextInteractionFlags(Qt::TextSelectableByMouse);
    aboutText->setText(
          "<p align=\"center\"><b>" + tr("MMapper Version %1").arg(QString(MMAPPER_VERSION))
        + "</b></p>"
        + "<p align=\"center\">"
#ifdef SVN_REVISION
        + tr("SVN revision %1").arg(QString::number(SVN_REVISION))
        + "<br>"
#endif
        + tr("Based on Qt %1 (%2 bit)").arg(QLatin1String(QT_VERSION_STR),
                                            QString::number(QSysInfo::WordSize))
        + "<br>"
        + tr("Built on %1 at %2").arg(QLatin1String(__DATE__),
                                      QLatin1String(__TIME__))
        + "</p>");

    /* Authors tab */
    authorsView->setOpenExternalLinks(true);
    authorsView->setHtml(tr(
        "Maintainer: Jahara (nschimme@gmail.com)<br>"
        "<br>"
        "<u>Special thanks to:</u><br>"
        "Alve for his great map engine<br>"
        "Caligor for starting the mmapper project<br>"
        "Azazello for his work with the Group Manager protocol<br>"
        "<br>"
        "<u>Contributors:</u><br>"
        "Jahara, Kalev, Azazello, Alve, Caligor, Kovis, Krush, and Korir<br>"
        "<br>"
        ));

    /* License tab */
    QFile f(":/COPYING");
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        licenseView->setText(tr("Unable to open file 'COPYING'."));
    } else {
        QTextStream ts(&f);
        licenseView->setText(ts.readAll());
    }
    setFixedFont(licenseView);
    licenseView->setMinimumWidth(600);

    setMaximumSize(sizeHint());
}

void AboutDialog::setFixedFont(QTextBrowser *browser)
{
    QFont fixed;
    fixed.setStyleHint(QFont::TypeWriter);
    fixed.setFamily("Courier");
    browser->setFont(fixed);
}
