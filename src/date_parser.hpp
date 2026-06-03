#pragma once

#include <QByteArray>
#include <QDate>

namespace date_parser {
    QDate from_jpg_buffer(const QByteArray& buffer);
    QDate from_mp4_buffer(const QByteArray& buffer);
    QDate from_file_name(const std::string& file_name);
}