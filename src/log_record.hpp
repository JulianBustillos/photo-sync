#pragma once

#include <QDateTime>
#include <QString>
#include <cstdint>

namespace logging {

    enum class LogLevel : std::uint8_t {
        Debug = 0,
        Info = 1,
        Warning = 2,
        Error = 3,
        Fatal = 4
    };

    struct LogRecord {
        QDateTime timestamp;
        LogLevel level;
        QString category;
        QString message;
    };

}