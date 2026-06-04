#include "logger.hpp"

namespace logging {
    Logger& Logger::instance() {
        static Logger logger;
        return logger;
    }

    void Logger::add_sink(std::shared_ptr<LogSink> sink) {
        QMutexLocker lock(&mutex_);
        sinks_.push_back(std::move(sink));
    }

    void Logger::log(const LogRecord& record) {
        QMutexLocker lock(&mutex_);
        for (auto& sink : sinks_) {
            if (sink->accepts(record.level)) {
                sink->write(record);
            }
        }
    }
}