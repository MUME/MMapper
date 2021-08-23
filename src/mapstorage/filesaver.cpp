// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Thomas Equeter <waba@waba.be> (Waba)

#include "filesaver.h"

#include <cstdio>
#include <stdexcept>
#include <QByteArray>
#include <QIODevice>

#include "../configuration/configuration.h"
#include "../global/TextUtils.h"
#include "../global/io.h"

static constexpr const bool USE_TMP_SUFFIX = CURRENT_PLATFORM != PlatformEnum::Windows;

static const char *const TMP_FILE_SUFFIX = ".tmp";
NODISCARD static auto maybe_add_suffix(const QString &filename)
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
        throw std::runtime_error(::toStdStringUtf8(m_file.errorString()));
    }
}

void FileSaver::close() noexcept(false)
{
    if (!m_file.isOpen()) {
        return;
    }

    m_file.flush();
    // REVISIT: check return value?
    MAYBE_UNUSED const auto ignored = ::io::fsync(m_file);
    remove_tmp_suffix(m_filename);
    m_file.close();
}
