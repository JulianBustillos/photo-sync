#pragma once
#include "FileData.hpp"
#include <QByteArray>

namespace DateParser {
bool fromJPGBuffer(const QByteArray &buffer, Date &date);
bool fromMP4Buffer(const QByteArray &buffer, Date &date);
bool fromFileName(const std::string &fileName, Date &date);
} // namespace DateParser