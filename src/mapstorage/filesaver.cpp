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

#include "filesaver.h"

#include <cstdio>
#include <stdexcept>
#include <QByteArray>
#include <QIODevice>

#include "../configuration/configuration.h"
#include "../global/io.h"

static constexpr const bool USE_TMP_SUFFIX = CURRENT_PLATFORM != Platform::Win32;

static const char *const TMP_FILE_SUFFIX = ".tmp";
auto maybe_add_suffix(const QString &filename)
{
    return USE_TMP_SUFFIX ? (filename + TMP_FILE_SUFFIX) : filename;
}

static void remove_tmp_suffix(const QString &filename) noexcept(false)
{
    if (!USE_TMP_SUFFIX)
        return;

    const auto from = QFile::encodeName(filename + TMP_FILE_SUFFIX);
    const auto to = QFile::encodeName(filename);
    if (::rename(from.data(), to.data()) == -1) {
        throw io::IOException::withCurrentErrno();
    }
}

FileSaver::~FileSaver()
{
    try {
        close();
    } catch (...) {
    }
}

void FileSaver::open(const QString &filename) noexcept(false)
{
    close();

    m_filename = filename;
    m_file.setFileName(maybe_add_suffix(filename));

    if (!m_file.open(QFile::WriteOnly)) {
        throw std::runtime_error(m_file.errorString().toStdString());
    }
}

void FileSaver::close() noexcept(false)
{
    if (!m_file.isOpen()) {
        return;
    }

    m_file.flush();
    ::io::fsync(m_file);
    remove_tmp_suffix(m_filename);
    m_file.close();
}
