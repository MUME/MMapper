// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include "ThemeManager.h"

#include "../configuration/configuration.h"
#include "../global/ConfigEnums.h"

#include <QApplication>
#include <QPalette>
#include <QStyleHints>

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
#include <QStyleHints>
#endif

#ifdef Q_OS_WIN
#include <windows.h>
#endif

ThemeManager::ThemeManager(QObject *const parent)
    : QObject(parent)
{
    setConfig().general.registerChangeCallback(m_lifetime, [this]() { applyTheme(); });

#if QT_VERSION < QT_VERSION_CHECK(6, 5, 0)
    if constexpr (CURRENT_PLATFORM == PlatformEnum::Windows) {
        qApp->installNativeEventFilter(this);
    }
#else
    connect(qApp->styleHints(), &QStyleHints::colorSchemeChanged, this, [this](Qt::ColorScheme) {
        if (getConfig().general.getTheme() == ThemeEnum::System) {
            applyTheme();
        }
    });
#endif

    applyTheme();
}

ThemeManager::~ThemeManager()
{
#if QT_VERSION < QT_VERSION_CHECK(6, 5, 0)
    if constexpr (CURRENT_PLATFORM == PlatformEnum::Windows) {
        qApp->removeNativeEventFilter(this);
    }
#endif
}

bool ThemeManager::nativeEventFilter(const QByteArray &eventType,
                                     void *message,
                                     qintptr * /*result */)
{
    if (eventType != "windows_generic_MSG")
        return false;

#ifdef Q_OS_WIN
    // REVISIT: Delete this after minimum Qt is 6.5
    MSG *msg = static_cast<MSG *>(message);
    if (msg->message == WM_SETTINGCHANGE && msg->lParam != 0) {
        const wchar_t *param = reinterpret_cast<const wchar_t *>(msg->lParam);
        if (QString::fromWCharArray(param) == "ImmersiveColorSet") {
            applyTheme();
        }
    }
#else
    std::ignore = message;
#endif

    return false;
}

void ThemeManager::applyTheme()
{
    const auto theme = getConfig().general.getTheme();
    if (theme == ThemeEnum::System) {
#if QT_VERSION < QT_VERSION_CHECK(6, 5, 0) && defined(Q_OS_WIN)
        const bool isDarkMode = std::invoke([]() {
            DWORD value = 1; // Default to light mode
            HKEY hKey;
            if (RegOpenKeyExW(HKEY_CURRENT_USER,
                              L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                              0,
                              KEY_READ,
                              &hKey)
                == ERROR_SUCCESS) {
                DWORD dataSize = sizeof(value);
                if (RegQueryValueExW(hKey,
                                     L"AppsUseLightTheme",
                                     nullptr,
                                     nullptr,
                                     reinterpret_cast<LPBYTE>(&value),
                                     &dataSize)
                    == ERROR_SUCCESS) {
                    RegCloseKey(hKey);
                    return value == 0; // 0 means dark mode
                }
                RegCloseKey(hKey);
            }
        });
        if (isDark()) {
            applyDarkPalette();
        } else {
            qApp->setPalette(QPalette());
            qApp->setStyle("Fusion");
        }
#else
        qApp->setPalette(QPalette());
        qApp->setStyle("Fusion");
#endif
    } else if (theme == ThemeEnum::Dark) {
        applyDarkPalette();
    } else {
        applyLightPalette();
    }
}

void ThemeManager::applyDarkPalette()
{
    QPalette dark;
    dark.setColor(QPalette::Window, QColor(53, 53, 53));
    dark.setColor(QPalette::WindowText, Qt::white);
    dark.setColor(QPalette::Base, QColor(25, 25, 25));
    dark.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    dark.setColor(QPalette::ToolTipBase, QColor(53, 53, 53));
    dark.setColor(QPalette::ToolTipText, Qt::white);
    dark.setColor(QPalette::Text, Qt::white);
    dark.setColor(QPalette::Button, QColor(53, 53, 53));
    dark.setColor(QPalette::ButtonText, Qt::white);
    dark.setColor(QPalette::BrightText, Qt::red);
    dark.setColor(QPalette::Highlight, QColor(142, 45, 197).lighter());
    dark.setColor(QPalette::HighlightedText, Qt::black);
    dark.setColor(QPalette::Disabled, QPalette::Text, Qt::darkGray);
    dark.setColor(QPalette::Disabled, QPalette::ButtonText, Qt::darkGray);

    qApp->setPalette(dark);
    qApp->setStyle("Fusion");
}

void ThemeManager::applyLightPalette()
{
    QPalette light;
    light.setColor(QPalette::Window, QColor(240, 240, 240));
    light.setColor(QPalette::WindowText, Qt::black);
    light.setColor(QPalette::Base, QColor(240, 240, 240));
    light.setColor(QPalette::AlternateBase, QColor(220, 220, 220));
    light.setColor(QPalette::ToolTipBase, QColor(240, 240, 240));
    light.setColor(QPalette::ToolTipText, Qt::black);
    light.setColor(QPalette::Text, Qt::black);
    light.setColor(QPalette::Button, QColor(240, 240, 240));
    light.setColor(QPalette::ButtonText, Qt::black);
    light.setColor(QPalette::BrightText, Qt::red);
    light.setColor(QPalette::Highlight, QColor(0, 120, 215));
    light.setColor(QPalette::HighlightedText, Qt::white);
    light.setColor(QPalette::Disabled, QPalette::Text, Qt::gray);
    light.setColor(QPalette::Disabled, QPalette::ButtonText, Qt::gray);

    qApp->setPalette(light);
    qApp->setStyle("Fusion");
}
