// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../global/RuleOf5.h"
#include "../global/logging.h"
#include "MapCanvasWindow.h"

#include <QOpenGLContext>

class NODISCARD MakeCurrentRaii final
{
private:
    QOpenGLWindow &m_glWindow;

public:
    explicit MakeCurrentRaii(QOpenGLWindow &window)
        : m_glWindow{window}
    {
        m_glWindow.makeCurrent();
    }
    ~MakeCurrentRaii() { m_glWindow.doneCurrent(); }

    DELETE_CTORS_AND_ASSIGN_OPS(MakeCurrentRaii);
};

void MapCanvasWindow::shuttingDown()
{
    qInfo() << MM_SOURCE_LOCATION().function_name();
    if (!m_core.isCleanedUp()) {
        cleanupOpenGL();
    }
}

void MapCanvasWindow::cleanupOpenGL()
{
    // Make sure the context is current and then explicitly destroy all
    // underlying OpenGL resources. hostCleanupGL() requires a current
    // context; only the host (us) knows how to make one current.
    MakeCurrentRaii makeCurrentRaii{*this};
    m_core.hostCleanupGL();
}

void MapCanvasWindow::initializeGL()
{
    if (!m_core.hostInitializeGL()) {
        // Failure was already handled synchronously by handleGlInitFailed()
        // (connected to MapCanvas::sig_glInitFailed with a direct
        // connection in the constructor).
        return;
    }

    // Clean up GL resources while the context is still current.
    // The destructor is too late — Qt destroys the context before ~MapCanvasWindow() runs.
    connect(context(),
            &QOpenGLContext::aboutToBeDestroyed,
            this,
            &MapCanvasWindow::cleanupOpenGL,
            Qt::DirectConnection);
}
