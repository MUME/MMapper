// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "HideQDebug.h"

#include "Badge.h"
#include "RAII.h"
#include "utils.h"

#include <cassert>
#include <iostream>
#include <mutex>
#include <stack>

#include <QDebug>

namespace mmqt {

class NODISCARD HideDebugPimpl final : public std::enable_shared_from_this<HideDebugPimpl>
{
private:
    static inline std::mutex g_mutex;
    static inline std::weak_ptr<HideDebugPimpl> g_top;

private:
    HideQDebugOptions m_options;
    QtMessageHandler m_next_handler = nullptr;
    std::weak_ptr<HideDebugPimpl> m_prev_pimpl;
    bool m_linked = false;

public:
    explicit HideDebugPimpl(Badge<HideDebugPimpl>, const HideQDebugOptions options)
        : m_options{options}
    {}
    ~HideDebugPimpl() { unlinkSelf(); }

private:
    NODISCARD static auto getTop_locked() { return g_top.lock(); }
    NODISCARD static auto getTop()
    {
        std::unique_lock<std::mutex> lock{g_mutex};
        return getTop_locked();
    }

private:
    void linkSelf_locked()
    {
        if (m_linked) {
            std::abort();
        }

        m_next_handler = qInstallMessageHandler(&HideDebugPimpl::messageOutput);
        m_prev_pimpl = std::exchange(g_top, shared_from_this());
        m_linked = true;

        assert(getTop_locked() == shared_from_this());
    }

public:
    void unlinkSelf()
    {
        std::unique_lock<std::mutex> lock{g_mutex};
        if (!m_linked) {
            return;
        }

        if (getTop_locked() != shared_from_this()) {
            std::abort();
        }

        m_linked = false;
        g_top = m_prev_pimpl;
        const auto tmp = qInstallMessageHandler(m_next_handler);
        if (tmp != &messageOutput) {
            std::abort();
        }
    }

private:
    void output(const QtMsgType type, const QMessageLogContext &context, const QString &msg)
    {
        RAIICallback eventually{[this]() {
            std::unique_lock<std::mutex> lock{g_mutex};
            g_top = shared_from_this();
        }};

        {
            std::unique_lock<std::mutex> lock{g_mutex};
            if (g_top.lock() != shared_from_this()) {
                std::abort();
            }

            switch (type) {
            case QtDebugMsg:
                if (m_options.hideDebug) {
                    return;
                }
                break;
            case QtInfoMsg:
                if (m_options.hideInfo) {
                    return;
                }
                break;
            case QtWarningMsg:
                if (m_options.hideWarning) {
                    return;
                }
                break;
            case QtCriticalMsg:
            case QtFatalMsg:
                break;
            }

            g_top = m_prev_pimpl;
        }

        m_next_handler(type, context, msg);
    }

public:
    NODISCARD static std::shared_ptr<HideDebugPimpl> alloc(const HideQDebugOptions options)
    {
        auto pSelf = std::make_shared<HideDebugPimpl>(Badge<HideDebugPimpl>(), options);
        auto &self = deref(pSelf);
        std::unique_lock<std::mutex> lock{g_mutex};
        self.linkSelf_locked();
        return pSelf;
    }
    static void messageOutput(const QtMsgType type,
                              const QMessageLogContext &context,
                              const QString &msg)
    {
        auto top = getTop();
        deref(top).output(type, context, msg);
    }
};

HideQDebug::HideQDebug(const HideQDebugOptions options)
    : m_pimpl{HideDebugPimpl::alloc(options)}
{}

HideQDebug::~HideQDebug()
{
    deref(m_pimpl).unlinkSelf();
}

} // namespace mmqt
