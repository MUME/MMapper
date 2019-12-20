// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Kalev Lember <kalev@smartlink.ee> (Kalev)

#include "aboutdialog.h"

#include <QString>
#include <QtConfig>
#include <QtGui>
#include <QtWidgets>

#include "../global/Version.h"

static QString getBuildInformation()
{
    const auto get_compiler = []() -> QString {
#ifdef __clang__
        return QString("Clang %1.%2.%3")
            .arg(__clang_major__)
            .arg(__clang_minor__)
            .arg(__clang_patchlevel__);
#elif __GNUC__
        return QString("GCC %1.%2.%3").arg(__GNUC__).arg(__GNUC_MINOR__).arg(__GNUC_PATCHLEVEL__);
#elif defined(_MSC_VER)
        return QString("MSVC %1").arg(_MSC_VER);
#else
        return "an unknown compiler";
#endif
    };

    return QString("Built on branch %1 using %2<br>")
        .arg(QLatin1String(getMMapperBranch()))
        .arg(get_compiler());
}

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowIcon(QIcon(":/icons/m.png"));
    setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    /* About tab */
    pixmapLabel->setPixmap(QPixmap(":/pixmaps/splash.png"));
    const auto about_text = []() -> QString {
        return "<p align=\"center\">"
               "<h3>"
               "<u>"
               + tr("MMapper %1").arg(QLatin1String(getMMapperVersion()))
               + "</h3>"
                 "</u>"
                 "</p>"
                 "<p align=\"center\">"
               + getBuildInformation()
               + tr("Based on Qt %1 (%2 bit)")
                     .arg(QT_VERSION_STR)
                     .arg(static_cast<size_t>(QSysInfo::WordSize))
               + "</p>";
    };
    aboutText->setText(about_text());

    /* Authors tab */
    authorsView->setHtml(tr(
        "<p>Maintainer: Jahara (please report bugs <a href=\"https://github.com/MUME/MMapper/issues\">here</a>)</p>"
        "<p><u>Special thanks to:</u><br>"
        "Alve for his great map engine<br>"
        "Caligor for starting the mmapper project<br>"
        "Azazello for creating the group manager</p>"
        "<p><u>Contributors:</u><br>"
        "Arfang, Ethorondil, Kalev, Korir, Kovis, Krush, Teoli, and Waba"
        "</p>"));

    /* Licenses tab */
    const auto loadLicenseResource = [](const QString &path) {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return QString("Unable to open resource '%1'.").arg(path);
        } else {
            QTextStream ts(&f);
            return ts.readAll();
        }
    };
    licenseView->setText(
        "<p>"
        "This program is free software; you can redistribute it and/or "
        "modify it under the terms of the GNU General Public License "
        "as published by the Free Software Foundation; either version 2 "
        "of the License, or (at your option) any later version."
        "</p><p>"
        "This program is distributed in the hope that it will be useful, "
        "but WITHOUT ANY WARRANTY; without even the implied warranty of "
        "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE."
        "</p><p>"
        "See the GNU General Public License for more details. "
        "</p>"
        "<hr/><h1>GNU General Public License 2.0</h1>"
        "<pre>"
        + loadLicenseResource(":/LICENSE.GPL2")
        + "</pre>"
          "<hr/><h1>GNU Lesser General Public License 2.1</h1>"
          "<p>Some versions of this product contains code from the "
          "following LGPLed libraries: "
          "<a href=\"https://github.com/jrfonseca/drmingw\">DrMingW</a></p>"
          "<pre>"
        + loadLicenseResource(":/LICENSE.LGPL")
        + "</pre>"
          "<hr/><h1>DejaVu Fonts License</h1>"
          "<p>This license applies to the file <code>src/resources/fonts/DejaVuSansMono.ttf</code></p>"
          "<pre>"
        + loadLicenseResource(":/fonts/LICENSE")
        + "</pre>"
          "<hr/><h1>MiniUPnPc License</h1>"
          "<p>Some versions of this product contains code from the "
          "<a href=\"https://github.com/miniupnp/miniupnp/\">MiniUPnPc</a>"
          " project.</p>"
          "<pre>"
        + loadLicenseResource(":/LICENSE.MINIUPNPC")
        + "</pre>"
          "<hr/><h1>GLM License</h1>"
          "<p>This product contains code from the "
          "<a href=\"https://glm.g-truc.net/\">OpenGL Mathematics (GLM)</a>"
          " project.</p>"
          "<pre>"
        + loadLicenseResource(":/LICENSE.GLM")
        + +"</pre>"
           "<hr/><h1>OpenSSL License</h1>"
           "<p>Some versions of this product contains code from the "
           "<a href=\"https://www.openssl.org/\">OpenSSL toolkit</a>"
           ".</p>"
           "<pre>"
        + loadLicenseResource(":/LICENSE.OPENSSL") + "</pre>");
    setFixedFont(licenseView);

    adjustSize();
}

void AboutDialog::setFixedFont(QTextBrowser *browser)
{
    QFont fixed;
    fixed.setStyleHint(QFont::TypeWriter);
    fixed.setFamily("Courier");
    browser->setFont(fixed);
}
