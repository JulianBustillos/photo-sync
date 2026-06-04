#pragma once

#include "log_record.hpp"

namespace logging {

    class LogSink {
    public:
        virtual ~LogSink() = default;

        void set_level(LogLevel level) {
            level_ = level;
        }

        [[nodiscard]] bool accepts(LogLevel level) const {
            return level >= level_;
        }

        virtual void write(const LogRecord& record) = 0;

    private:
        LogLevel level_ = LogLevel::Warning;
    };

}