// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include "WinDarkMode.h"

#include "../global/ConfigConsts-Computed.h"

#include <QApplication>
#include <QDebug>
#include <QPalette>
#include <QWidget>

#ifdef WIN32
#include <windows.h>
#endif

WinDarkMode::WinDarkMode(QObject *const parent)
    : QObject(parent)
{
    if constexpr (CURRENT_PLATFORM == PlatformEnum::Windows) {
        qApp->installNativeEventFilter(this);
        applyCurrentPalette();
    }
}

WinDarkMode::~WinDarkMode()
{
    if constexpr (CURRENT_PLATFORM == PlatformEnum::Windows) {
        qApp->removeNativeEventFilter(this);
    }
}

bool WinDarkMode::nativeEventFilter(const QByteArray &eventType,
                                    void *message,
                                    qintptr * /*result */)
{
    if (eventType != "windows_generic_MSG")
        return false;

#ifdef WIN32
    MSG *msg = static_cast<MSG *>(message);
    if (msg->message == WM_SETTINGCHANGE && msg->lParam != 0) {
        const wchar_t *param = reinterpret_cast<const wchar_t *>(msg->lParam);
        if (QString::fromWCharArray(param) == "ImmersiveColorSet") {
            applyCurrentPalette();
            emit sig_darkModeChanged(isDarkMode());
        }
    }
#else
    std::ignore = message;
#endif

    return false;
}

bool WinDarkMode::isDarkMode()
{
#ifdef WIN32
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
#endif
    return false;
}

void WinDarkMode::applyCurrentPalette()
{
    if (isDarkMode()) {
        applyDarkPalette();
    } else {
        applyLightPalette();
    }
}

void WinDarkMode::applyDarkPalette()
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

    qApp->setPalette(dark);
    qApp->setStyle("Fusion");
}

void WinDarkMode::applyLightPalette()
{
    qApp->setPalette(QPalette());
    qApp->setStyle("Fusion");
}
