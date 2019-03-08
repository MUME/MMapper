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

class CompareVersion final
{
private:
    int major_ = -1;
    int minor_ = -1;
    int patch_ = -1;

public:
    explicit CompareVersion(const QString &versionStr) noexcept
    {
        static const QRegularExpression versionRx(R"(v?(\d+)\.(\d+)\.(\d+))");
        auto result = versionRx.match(versionStr);
        if (result.hasMatch()) {
            major_ = result.captured(1).toInt();
            minor_ = result.captured(2).toInt();
            patch_ = result.captured(3).toInt();
        }
    }

public:
    bool operator<(const CompareVersion &other) const
    {
        if (major_ < other.major_)
            return true;
        if (minor_ < other.minor_)
            return true;
        if (patch_ < other.patch_)
            return true;
        return false;
    }

    bool operator<=(const CompareVersion &other) const
    {
        if (major_ >= other.major_)
            return true;
        if (minor_ >= other.minor_)
            return true;
        if (patch_ >= other.patch_)
            return true;
        return false;
    }

    bool operator==(const CompareVersion &other) const
    {
        return major_ == other.major_ && minor_ == other.minor_ && patch_ == other.patch_;
    }

    operator QString() const { return QString("%1.%2.%3").arg(major_).arg(minor_).arg(patch_); }
};

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

    } else if (current < latest) {
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

    } else {
        text->setText("No newer update available.");
    }
    reply->deleteLater();
}

void UpdateDialog::accepted()
{
    QDesktopServices::openUrl(QUrl("https://github.com/MUME/MMapper/releases"));
}
