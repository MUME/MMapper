// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Kalev Lember <kalev@smartlink.ee> (Kalev)

#include "aboutdialog.h"

#include "../global/ConfigConsts-Computed.h"
#include "../global/ConfigEnums.h"
#include "../global/Version.h"

#include <QString>
#include <QtConfig>
#include <QtGui>
#include <QtWidgets>

namespace {
struct LicenseInfo
{
    QString title;
    QString introText;
    QString resourcePath;
};

NODISCARD QString getBuildInformation()
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
        .arg(QString::fromUtf8(getMMapperBranch()), get_compiler());
}

} // namespace

AboutDialog::AboutDialog(QWidget *const parent)
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
               + QString("MMapper %1").arg(QString::fromUtf8(getMMapperVersion()))
               + "</h3>"
                 "</u>"
                 "</p>"
                 "<p align=\"center\">"
               + getBuildInformation()
               + QString("Based on Qt %1 (%2 bit)")
                     .arg(qVersion())
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
        "Arfang, Cosmos, Cuantar, Elval, Kalev, Korir, Kovis, Krush, Mirnir, Taryn, Teoli, and Waba"
        "</p>"));

    const auto loadResource = [](const QString &path) {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return QString("Unable to open resource '%1'.").arg(path);
        } else {
            QTextStream ts(&f);
            return ts.readAll();
        }
    };

    /* Licenses tab */
    QList<LicenseInfo> licenses;
    licenses.append({"GNU General Public License 2.0",
                     "<p>"
                     "This program is free software; you can redistribute it and/or "
                     "modify it under the terms of the GNU General Public License "
                     "as published by the Free Software Foundation; either version 2 "
                     "of the License, or (at your option) any later version."
                     "</p>"
                     "<p>"
                     "This program is distributed in the hope that it will be useful, "
                     "but WITHOUT ANY WARRANTY; without even the implied warranty of "
                     "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE."
                     "</p>"
                     "<p>"
                     "See the GNU General Public License for more details. "
                     "</p>",
                     ":/LICENSE.GPL2"});

    licenses.append({"DejaVu Fonts License",
                     "<p>"
                     "This license applies to the file "
                     "<code>src/resources/fonts/DejaVuSansMono.ttf</code>"
                     "</p>",
                     ":/fonts/LICENSE"});

    licenses.append({"GLM License",
                     "<p>"
                     "This product contains code from the "
                     "<a href=\"https://glm.g-truc.net/\">OpenGL Mathematics (GLM)</a>"
                     " project."
                     "</p>",
                     ":/LICENSE.GLM"});

    licenses.append({"QtKeychain License",
                     "<p>"
                     "This product contains code from the "
                     "<a href=\"https://github.com/frankosterfeld/qtkeychain\">QtKeychain</a>"
                     " project."
                     "</p>",
                     ":/LICENSE.QTKEYCHAIN"});

    licenses.append({"OpenSSL License",
                     "<p>"
                     "Some versions of this product contains code from the "
                     "<a href=\"https://www.openssl.org/\">OpenSSL toolkit</a>."
                     "</p>",
                     ":/LICENSE.OPENSSL"});

    licenses.append({"Boost Software License 1.0",
                     "<p>"
                     "This product contains code from the "
                     "<a href=\"https://github.com/arximboldi/immer\">immer</a>"
                     " project."
                     "</p>",
                     ":/LICENSE.BOOST"});

    if constexpr (CURRENT_PLATFORM == PlatformEnum::Windows) {
        licenses.append({"GNU Lesser General Public License 2.1",
                         "<p>"
                         "Some versions of this product contains code from the "
                         "following LGPLed libraries: "
                         "<a href=\"https://github.com/jrfonseca/drmingw\">DrMingW</a>"
                         "</p>",
                         ":/LICENSE.LGPL"});
    }

    for (const auto &license : licenses) {
        auto *titleLabel = new QLabel(QStringLiteral("<h2>%1</h2>").arg(license.title));
        titleLabel->setTextFormat(Qt::RichText);
        licenseLayout->addWidget(titleLabel);

        if (!license.introText.isEmpty()) {
            auto *introLabel = new QLabel(license.introText);
            introLabel->setWordWrap(true);
            introLabel->setTextFormat(Qt::RichText);
            licenseLayout->addWidget(introLabel);
        }

        auto *textEdit = new QTextEdit;
        textEdit->setReadOnly(true);
        textEdit->setPlainText(loadResource(license.resourcePath));
        textEdit->setFixedHeight(160);
        licenseLayout->addWidget(textEdit);

        auto *hr = new QFrame;
        hr->setFrameShape(QFrame::HLine);
        hr->setFrameShadow(QFrame::Sunken);
        licenseLayout->addWidget(hr);
    }

    adjustSize();
}

void AboutDialog::setFixedFont(QTextBrowser *browser)
{
    QFont fixed;
    fixed.setStyleHint(QFont::TypeWriter);
    fixed.setFamily("Courier");
    browser->setFont(fixed);
}
