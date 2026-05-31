#pragma once
#include "file_data.hpp"

#include <QByteArray>

namespace date_parser {
    bool from_jpg_buffer(const QByteArray& buffer, Date& date);
    bool from_mp4_buffer(const QByteArray& buffer, Date& date);
    bool from_file_name(const std::string& file_name, Date& date);
}