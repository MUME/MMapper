// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "UpdateDialog.h"

#include "../configuration/configuration.h"
#include "../global/TextUtils.h"
#include "../global/Version.h"

#include <QDesktopServices>
#include <QGridLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QPushButton>
#include <QRegularExpression>

static constexpr const char *APPIMAGE_KEY = "APPIMAGE";

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

QString CompareVersion::toQString() const
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
    connect(buttonBox, &QDialogButtonBox::accepted, this, [this]() {
        if (QDesktopServices::openUrl(downloadUrl))
            close();
    });
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
    const QString answer = reply->readAll();
    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(answer.toUtf8(), &error);
    if (doc.isNull()) {
        qWarning() << error.errorString();
        return;
    }
    if (!doc.isObject()) {
        qWarning() << answer;
        return;
    }

    const QJsonObject obj = doc.object();
    downloadUrl = [&obj]() -> QString {
        // Regular expressions to detect if this download matches the current install
        const auto platformRegex = QRegularExpression(
            []() -> const char * {
                if constexpr (CURRENT_PLATFORM == PlatformEnum::Mac)
                    return R"(^.+\.dmg$)";
                if constexpr (CURRENT_PLATFORM == PlatformEnum::Linux)
                    return R"(^.+\.(deb|AppImage)$)";
                if constexpr (CURRENT_PLATFORM == PlatformEnum::Windows)
                    return R"(^.+\.exe$)";
                abort();
            }(),
            QRegularExpression::PatternOption::CaseInsensitiveOption);
        const auto environmentRegex = QRegularExpression(
            []() -> const char * {
                if constexpr (CURRENT_ENVIRONMENT == EnvironmentEnum::Env32Bit)
                    return R"((arm(?!64)|armhf|i386|x86(?!_64)))";
                if constexpr (CURRENT_ENVIRONMENT == EnvironmentEnum::Env64Bit)
                    return R"((aarch64|amd64|arm64|x86_64|x64))";
                abort();
            }(),
            QRegularExpression::PatternOption::CaseInsensitiveOption);

        if (obj.contains("assets") && obj["assets"].isArray()) {
            for (auto item : obj["assets"].toArray()) {
                const QJsonObject itemObj = item.toObject();
                if (itemObj.contains("name") && itemObj["name"].isString()
                    && itemObj.contains("browser_download_url")
                    && itemObj["browser_download_url"].isString()) {
                    const QString name = itemObj["name"].toString();
                    if (!name.contains(platformRegex) || !name.contains(environmentRegex))
                        continue;
                    if constexpr (CURRENT_PLATFORM == PlatformEnum::Linux) {
                        // If MMapper is an AppImage then only select AppImage assets
                        // Similarly, if MMapper is not an AppImage then skip AppImage assets
                        const bool assetAppImage
                            = name.contains("AppImage", Qt::CaseSensitivity::CaseInsensitive);
                        const bool envAppImage = qgetenv(APPIMAGE_KEY) != nullptr;
                        if ((envAppImage && !assetAppImage) || (!envAppImage && assetAppImage))
                            continue;
                    }
                    return itemObj["browser_download_url"].toString();
                }
            }
        }
        if (obj.contains("html_url") && obj["html_url"].isString())
            return obj["html_url"].toString();
        return "https://github.com/MUME/MMapper/releases";
    }();

    if (!obj.contains("tag_name") || !obj["tag_name"].isString())
        return;
    const QString latestTag = obj["tag_name"].toString();
    const QString currentVersion = QLatin1String(getMMapperVersion());
    const CompareVersion latest(latestTag);
    static const CompareVersion current(currentVersion);
    qInfo() << "Updater comparing: CURRENT=" << current << "LATEST=" << latest
            << "URL=" << downloadUrl;
    if (current == latest) {
        text->setText("You are up to date!");

    } else if (current > latest) {
        text->setText("No newer update available.");

    } else {
        text->setText(QString("A new version of MMapper is available!"
                              "\n"
                              "\n"
                              "Press 'Upgrade' to download MMapper %1!")
                          .arg(latestTag));
        buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);

        // Show and raise the dialog in case this is the startup check
        show();
        raise();
        activateWindow();
    }
    reply->deleteLater();
}
