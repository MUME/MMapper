/************************************************************************
**
** Authors:   Nils Schimmelmann <nschimme@gmail.com> (Jahara)
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

#include "UpdateDialog.h"

#include <QDesktopServices>
#include <QGridLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QPushButton>
#include <QRegularExpression>

CompareVersion::CompareVersion(const QString &versionStr) noexcept
{
    static const QRegularExpression versionRx(R"(v?(\d+)\.(\d+)\.(\d+))");
    auto result = versionRx.match(versionStr);
    if (result.hasMatch()) {
        parts[0] = result.captured(1).toInt();
        parts[1] = result.captured(2).toInt();
        parts[2] = result.captured(3).toInt();
    }
}

bool CompareVersion::operator>(const CompareVersion &other) const
{
    for (size_t i = 0; i < parts.size(); i++) {
        auto myPart = parts.at(i);
        auto otherPart = other.parts.at(i);
        if (myPart == otherPart)
            continue;
        if (myPart > otherPart)
            return true;
        break;
    }
    return false;
}

bool CompareVersion::operator==(const CompareVersion &other) const
{
    return parts.at(0) == other.parts.at(0) && parts.at(1) == other.parts.at(1)
           && parts.at(2) == other.parts.at(2);
}

CompareVersion::operator QString() const
{
    return QString("%1.%2.%3").arg(parts.at(0)).arg(parts.at(1)).arg(parts.at(2));
}

UpdateDialog::UpdateDialog(QWidget *const parent)
    : QDialog(parent)
{
    setWindowTitle("MMapper Updater");
    setWindowIcon(QIcon(":/icons/m.png"));

    text = new QLabel(this);
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("&Upgrade"));
    connect(buttonBox, &QDialogButtonBox::accepted, this, &UpdateDialog::accepted);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &UpdateDialog::reject);

    QGridLayout *mainLayout = new QGridLayout(this);
    mainLayout->addWidget(text);
    mainLayout->addWidget(buttonBox);

    connect(&manager, &QNetworkAccessManager::finished, this, &UpdateDialog::managerFinished);
}

void UpdateDialog::open()
{
    text->setText(tr("Checking for new version..."));
    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    QNetworkRequest request(QUrl("https://api.github.com/repos/mume/mmapper/releases/latest"));
    request.setHeader(QNetworkRequest::ServerHeader, "application/json");
    manager.get(request);
}

void UpdateDialog::managerFinished(QNetworkReply *reply)
{
    // REVISIT: Timeouts, errors, etc
    if (reply->error()) {
        qWarning() << reply->errorString();
        return;
    }
    QString answer = reply->readAll();
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(answer.toLatin1(), &error);
    if (doc.isNull()) {
        qWarning() << error.errorString();
        return;
    }
    if (!doc.isObject()) {
        qWarning() << answer;
        return;
    }

    QJsonObject obj = doc.object();
    const QString latestTag = obj["tag_name"].toString();
    const QString currentVersion = QLatin1Literal(MMAPPER_VERSION);
    const CompareVersion latest(latestTag);
    static const CompareVersion current(currentVersion);
    qDebug() << "Updater comparing: CURRENT=" << current << "LATEST=" << latest;
    if (current == latest) {
        text->setText("You are up to date!");

    } else if (current > latest) {
        text->setText("No newer update available.");

    } else {
        text->setText(QString("A new version of MMapper is available!"
                              "\n"
                              "\n"
                              "Press 'Upgrade' to grab MMapper %1!")
                          .arg(latestTag));
        buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);

        // Show and raise the dialog in case this is the startup check
        show();
        raise();
        activateWindow();
    }
    reply->deleteLater();
}

void UpdateDialog::accepted()
{
    QDesktopServices::openUrl(QUrl("https://github.com/MUME/MMapper/releases"));
}
