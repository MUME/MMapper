#pragma once
/************************************************************************
**
** Authors:   Thomas Equeter <waba@waba.be>
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

#ifndef INCLUDED_FILESAVER_H
#define INCLUDED_FILESAVER_H

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
    explicit FileSaver() = default;
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

#endif /* INCLUDED_FILESAVER_H */
