#include "console_log_sink.hpp"

namespace logging {

    ConsoleLogSink::ConsoleLogSink(QObject* parent) : QObject(parent) {}

    void ConsoleLogSink::write(const LogRecord& record) {
        emit log_received(record);
    }

}