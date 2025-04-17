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
static constexpr const char *FLATPAK_KEY = "container";

CompareVersion::CompareVersion(const QString &versionStr) noexcept
{
    static const QRegularExpression versionRx(R"(v?(\d+)\.(\d+)\.(\d+))");
    auto result = versionRx.match(versionStr);
    if (result.hasMatch()) {
        m_parts[0] = result.captured(1).toInt();
        m_parts[1] = result.captured(2).toInt();
        m_parts[2] = result.captured(3).toInt();
    }
}

bool CompareVersion::operator>(const CompareVersion &other) const
{
    for (size_t i = 0; i < m_parts.size(); ++i) {
        auto myPart = m_parts.at(i);
        auto otherPart = other.m_parts.at(i);
        if (myPart == otherPart) {
            continue;
        }
        if (myPart > otherPart) {
            return true;
        }
        break;
    }
    return false;
}

bool CompareVersion::operator==(const CompareVersion &other) const
{
    return m_parts == other.m_parts;
}

QString CompareVersion::toQString() const
{
    const auto &parts = m_parts;
    return QString("%1.%2.%3").arg(parts.at(0)).arg(parts.at(1)).arg(parts.at(2));
}

UpdateDialog::UpdateDialog(QWidget *const parent)
    : QDialog(parent)
{
    setWindowTitle("MMapper Updater");
    setWindowIcon(QIcon(":/icons/m.png"));

    m_text = new QLabel(this);
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_buttonBox->button(QDialogButtonBox::Ok)->setText(tr("&Upgrade"));
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &UpdateDialog::accepted);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, [this]() {
        if (QDesktopServices::openUrl(m_downloadUrl)) {
            close();
        }
    });
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &UpdateDialog::reject);

    QGridLayout *mainLayout = new QGridLayout(this);
    mainLayout->addWidget(m_text);
    mainLayout->addWidget(m_buttonBox);

    connect(&m_manager, &QNetworkAccessManager::finished, this, &UpdateDialog::managerFinished);
}

void UpdateDialog::open()
{
    m_text->setText(tr("Checking for new version..."));
    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    QNetworkRequest request(QUrl("https://api.github.com/repos/mume/mmapper/releases/latest"));
    request.setHeader(QNetworkRequest::ServerHeader, "application/json");
    m_manager.get(request);
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
    m_downloadUrl = [&obj]() -> QString {
        // Compile platform-specific regex
        static const auto platformRegex = QRegularExpression(
            []() -> const char * {
                if constexpr (CURRENT_PLATFORM == PlatformEnum::Mac) {
                    return R"(^.+\.dmg$)";
                } else if constexpr (CURRENT_PLATFORM == PlatformEnum::Linux) {
                    return R"(^.+\.(deb|AppImage|flatpak)$)";
                } else if constexpr (CURRENT_PLATFORM == PlatformEnum::Windows) {
                    return R"(^.+\.exe$)";
                }
                abort();
            }(),
            QRegularExpression::CaseInsensitiveOption);

        // Compile architecture/environment-specific regex
        static const auto environmentRegex = QRegularExpression(
            []() -> const char * {
                if constexpr (CURRENT_ENVIRONMENT == EnvironmentEnum::Env32Bit) {
                    return R"((arm(?!64)|armhf|i386|x86(?!_64)))";
                } else if constexpr (CURRENT_ENVIRONMENT == EnvironmentEnum::Env64Bit) {
                    return R"((aarch64|amd64|arm64|x86_64|x64))";
                }
                abort();
            }(),
            QRegularExpression::CaseInsensitiveOption);

        const auto assets = obj.value("assets").toArray();
        for (const auto &item : assets) {
            const auto asset = item.toObject();
            const QString name = asset.value("name").toString();
            const QString url = asset.value("browser_download_url").toString();

            if (name.isEmpty() || url.isEmpty()) {
                continue;
            }

            if (!name.contains(platformRegex) || !name.contains(environmentRegex)) {
                continue;
            }

            if constexpr (CURRENT_PLATFORM == PlatformEnum::Linux) {
                const bool isAssetAppImage = name.contains("AppImage", Qt::CaseInsensitive);
                const bool isEnvAppImage = qEnvironmentVariableIsSet(APPIMAGE_KEY);
                if (isAssetAppImage != isEnvAppImage) {
                    continue;
                }

                const bool isAssetFlatpak = name.contains("flatpak", Qt::CaseInsensitive);
                const bool isEnvFlatpak = qEnvironmentVariableIsSet(FLATPAK_KEY);
                if (isAssetFlatpak != isEnvFlatpak) {
                    continue;
                }
            }

            return url;
        }

        const QString fallbackUrl = obj.value("html_url").toString();
        if (!fallbackUrl.isEmpty()) {
            return fallbackUrl;
        }

        return "https://github.com/MUME/MMapper/releases";
    }();

    if (!obj.contains("tag_name") || !obj["tag_name"].isString()) {
        return;
    }
    const QString latestTag = obj["tag_name"].toString();
    const QString currentVersion = QString::fromUtf8(getMMapperVersion());
    const CompareVersion latest(latestTag);
    static const CompareVersion current(currentVersion);
    qInfo() << "Updater comparing: CURRENT=" << current << "LATEST=" << latest
            << "URL=" << m_downloadUrl;
    if (current == latest) {
        m_text->setText("You are up to date!");

    } else if (current > latest) {
        m_text->setText("No newer update available.");

    } else {
        m_text->setText(QString("A new version of MMapper is available!"
                                "\n"
                                "\n"
                                "Press 'Upgrade' to download MMapper %1!")
                            .arg(latestTag));
        m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);

        // Show and raise the dialog in case this is the startup check
        show();
        raise();
        activateWindow();
    }
    reply->deleteLater();
}
