#pragma once

#include "log_sink.hpp"

#include <QFile>

namespace logging {

    class RotatingFileSink final : public LogSink {
    public:
        explicit RotatingFileSink(const QString& path);

        void write(const LogRecord& record) override;

    private:
        static QString format_record(const LogRecord& record);

        void rotate_if_needed();

        QString file_path_;
        QFile file_;
        qint64 max_size_bytes_ = static_cast<qint64>(1 * 1024 * 1024);
        int max_files_ = 1;
    };

}
