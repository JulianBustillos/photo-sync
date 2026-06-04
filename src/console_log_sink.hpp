
#pragma once

#include "log_record.hpp"
#include "log_sink.hpp"

#include <QObject>

namespace logging {

    class ConsoleLogSink final : public LogSink {
        Q_OBJECT

    public:
        void write(const LogRecord& record) override;

    signals:
        void log_received(const logging::LogRecord& record);
    };

}