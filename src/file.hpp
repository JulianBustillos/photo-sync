#pragma once

#include "sort_mode.hpp"

#include <QByteArray>
#include <QFileInfo>
#include <QString>

namespace file {
    struct Fingerprint {
        Fingerprint(const QFileInfo& info) : info(info) {};

        QFileInfo info;
        QByteArray checksum;
    };

    struct Entry {
        Entry(const QFileInfo& info, const QDate& date) : info(info), date(date) {};

        QFileInfo info;
        QDate date;
    };

    QString to_path(const QDate& date, SortMode mode);
}