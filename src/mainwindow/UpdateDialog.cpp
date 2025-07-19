// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "UpdateDialog.h"

#include "../configuration/configuration.h"
#include "../global/RAII.h"
#include "../global/Version.h"

#include <algorithm>
#include <array>
#include <utility>

#include <QDesktopServices>
#include <QGridLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QPushButton>
#include <QRegularExpression>
#include <QSysInfo>

static constexpr const char *APPIMAGE_KEY = "APPIMAGE";
static constexpr const char *FLATPAK_KEY = "container";

namespace { // anonymous

NODISCARD const char *getArchitectureRegexPattern()
{
    // See Qt documentation for expected keys
    const std::array<std::pair<const char *, const char *>, 4> archPatterns = {
        {{"arm64", "(arm64|aarch64)"},
         {"x86_64", "(x86_64|amd64|x64)"},
         {"i386", "(i386|x86(?!_64))"},
         {"arm", "(arm(?!64)|armhf)"}}};

    auto findPattern = [&](const QString &arch) -> const char * {
        auto it = std::find_if(archPatterns.begin(), archPatterns.end(), [&arch](const auto &pair) {
            return mmqt::toStdStringUtf8(arch) == pair.first;
        });
        return (it != archPatterns.end()) ? it->second : nullptr;
    };

    if (auto pattern = findPattern(QSysInfo::currentCpuArchitecture())) {
        return pattern;
    }
    if (auto fallback = findPattern(QSysInfo::buildCpuArchitecture())) {
        return fallback;
    }
    abort();
}

} // namespace

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

    auto url = isMMapperBeta()
                   ? QStringLiteral("https://api.github.com/repos/mume/mmapper/git/ref/tags/beta")
                   : QStringLiteral("https://api.github.com/repos/mume/mmapper/releases/latest");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ServerHeader, "application/json");
    m_manager.get(request);
}

void UpdateDialog::setUpdateStatus(const QString &message,
                                   bool enableUpgradeButton,
                                   bool showAndUpdateDialog)
{
    m_text->setText(message);
    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(enableUpgradeButton);
    if (showAndUpdateDialog) {
        show();
        raise();
        activateWindow();
    }
}

QString UpdateDialog::findDownloadUrlForRelease(const QJsonObject &releaseObject) const
{
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

    // Compile architecture-specific regex
    static const auto archRegex = QRegularExpression(getArchitectureRegexPattern(),
                                                     QRegularExpression::CaseInsensitiveOption);

    const auto assets = releaseObject.value("assets").toArray();
    for (const auto item : assets) {
        const auto asset = item.toObject();
        const QString name = asset.value("name").toString();
        const QString url = asset.value("browser_download_url").toString();

        if (name.isEmpty() || url.isEmpty()) {
            continue;
        }

        if (!name.contains(platformRegex) || !name.contains(archRegex)) {
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

    const QString fallbackUrl = releaseObject.value("html_url").toString();
    if (!fallbackUrl.isEmpty()) {
        return fallbackUrl;
    }

    return "https://github.com/MUME/MMapper/releases";
}

void UpdateDialog::managerFinished(QNetworkReply *reply)
{
    const RAIICallback deleteLaterRAII{[&reply]() { reply->deleteLater(); }};
    // REVISIT: Timeouts, errors, etc
    if (reply->error() != QNetworkReply::NoError) {
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

    if (isMMapperBeta() && reply->request().url().toString().contains("/ref/tags/")) {
        const QJsonObject objNode = obj.value("object").toObject();
        const QString remoteCommitHash = objNode.value("sha").toString();
        const QString localCommitHash = []() -> QString {
            static const QRegularExpression hashRegex(R"(-g([0-9a-fA-F]+)$)");
            QRegularExpressionMatch match = hashRegex.match(QString::fromUtf8(getMMapperVersion()));
            if (match.hasMatch()) {
                return match.captured(1);
            }
            return "";
        }();
        qInfo() << "Updater comparing: CURRENT=" << localCommitHash
                << "LATEST=" << remoteCommitHash.left(10);
        if (!localCommitHash.isEmpty() && remoteCommitHash.startsWith(localCommitHash)) {
            setUpdateStatus(tr("You are on the latest beta!"), false, false);
            return;
        }

        QNetworkRequest request(
            QStringLiteral("https://api.github.com/repos/mume/mmapper/releases/tags/beta"));
        request.setHeader(QNetworkRequest::ServerHeader, "application/json");
        m_manager.get(request);
        return;
    }

    QString latestTag;
    if (!isMMapperBeta()) {
        if (!obj.contains("tag_name") || !obj["tag_name"].isString()) {
            qWarning() << "Release 'tag_name' is missing or not a string.";
            setUpdateStatus(tr("Could not determine release version details."), false, false);
            return;
        }
        latestTag = obj["tag_name"].toString();
        const QString currentVersion = QString::fromUtf8(getMMapperVersion());
        const CompareVersion latest(latestTag);
        static CompareVersion current(currentVersion);
        qInfo() << "Updater comparing: CURRENT=" << current << "LATEST=" << latest;
        if (current == latest) {
            setUpdateStatus(tr("You are up to date!"), false, false);
            return;
        } else if (current > latest) {
            setUpdateStatus(tr("No newer update available."), false, false);
            return;
        }
    }
    m_downloadUrl = findDownloadUrlForRelease(obj);
    setUpdateStatus(QString("A new %1version of MMapper is available!"
                            "\n"
                            "\n"
                            "Press 'Upgrade' to download %2!")
                        .arg(isMMapperBeta() ? "beta " : "", isMMapperBeta() ? "it" : latestTag),
                    true,
                    true);
}
