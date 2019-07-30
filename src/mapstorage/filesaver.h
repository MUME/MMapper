#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Thomas Equeter <waba@waba.be> (Waba)

#include <QFile>
#include <QString>

#include "../global/RuleOf5.h"

/*! \brief Save to a file in an atomic way.
 *
 * Currently this does not work on Windows (where a simple file overwriting is
 * then performed).
 */
class FileSaver final
{
    QString m_filename{};
    QFile m_file{}; // disables copying

public:
    FileSaver() = default;
    ~FileSaver();

public:
    DELETE_CTORS_AND_ASSIGN_OPS(FileSaver);

public:
    QFile &file() { return m_file; }

    /*! \exception std::runtime_error if the file can't be opened or a currently
     * open file can't be closed.
     */
    void open(const QString &filename) noexcept(false);

    /*! \exception std::runtime_error if the file can't be safely closed.
     */
    void close() noexcept(false);
};
