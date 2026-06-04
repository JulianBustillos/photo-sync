#pragma once

#include "log_sink.hpp"

#include <QMutex>
#include <memory>
#include <vector>

namespace logging {
    class Logger {
    public:
        static Logger& instance();

        void add_sink(std::shared_ptr<LogSink> sink);

        void log(const LogRecord& record);

    private:
        Logger() = default;

        QMutex mutex_;
        std::vector<std::shared_ptr<LogSink>> sinks_;
    };
}