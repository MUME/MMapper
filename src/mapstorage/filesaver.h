#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Thomas Equeter <waba@waba.be> (Waba)

#include "../global/RuleOf5.h"
#include "../global/macros.h"

#include <QFile>
#include <QString>

/*! \brief Save to a file in an atomic way.
 *
 * Currently this does not work on Windows (where a simple file overwriting is
 * then performed).
 */
class NODISCARD FileSaver final
{
private:
    QString m_filename;
    QFile m_file; // old comment says "disables copying" ... why? and how?

public:
    FileSaver() = default;
    ~FileSaver();

public:
    DELETE_CTORS_AND_ASSIGN_OPS(FileSaver);

public:
    NODISCARD QFile &file() { return m_file; }

    /*! \exception std::runtime_error if the file can't be opened or a currently
     * open file can't be closed.
     */
    void open(const QString &filename) noexcept(false);

    /*! \exception std::runtime_error if the file can't be safely closed.
     */
    void close() noexcept(false);
};
