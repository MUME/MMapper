// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Thomas Equeter <waba@waba.be> (Waba)

#include "filesaver.h"

#include "../configuration/configuration.h"
#include "../global/TextUtils.h"
#include "../global/io.h"

#include <cstdio>
#include <stdexcept>
#include <tuple>

#include <QIODevice>

static const char *const TMP_FILE_SUFFIX = ".tmp";
NODISCARD static auto maybe_add_suffix(const QString &filename)
{
    return filename + TMP_FILE_SUFFIX;
}

static void remove_tmp_suffix(const QString &filename) CAN_THROW
{
    const QString from = filename + TMP_FILE_SUFFIX;
    const QString to = filename;

    io::rename(from, to);
}

FileSaver::~FileSaver()
{
    try {
        close();
    } catch (...) {
    }
}

void FileSaver::open(const QString &filename) CAN_THROW
{
    close();

    m_filename = filename;

    auto &file = deref(m_file);
    file.setFileName(maybe_add_suffix(filename));

    if (!file.open(QFile::WriteOnly)) {
        throw std::runtime_error(mmqt::toStdStringUtf8(file.errorString()));
    }
}

void FileSaver::close() CAN_THROW
{
    auto &file = deref(m_file);
    if (!file.isOpen()) {
        return;
    }

    file.flush();
    // REVISIT: check return value?
    std::ignore = ::io::fsync(file);
    remove_tmp_suffix(m_filename);
    file.close();
}
