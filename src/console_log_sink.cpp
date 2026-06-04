#include "console_log_sink.hpp"

namespace logging {

    void ConsoleLogSink::write(const LogRecord& record) {
        emit log_received(record);
    }

}