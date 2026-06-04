#pragma once

#include "log_record.hpp"

namespace logging {

    class LogSink : public QObject {
        Q_OBJECT

    public:
        LogSink() : QObject(nullptr) {};
        virtual ~LogSink() = default;
        LogSink(const LogSink&) = delete;
        LogSink& operator=(const LogSink&) = delete;
        LogSink(LogSink&&) noexcept = delete;
        LogSink& operator=(LogSink&&) noexcept = delete;

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