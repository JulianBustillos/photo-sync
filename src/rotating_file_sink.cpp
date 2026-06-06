#include "rotating_file_sink.hpp"

#include <QFileInfo>
#include <QTextStream>

namespace logging {

    RotatingFileSink::RotatingFileSink(const QString& path)
        : file_path_(std::move(path + "/log.txt")), file_(file_path_) {
        bool opened = file_.open(QIODevice::Append | QIODevice::Text);
    }

    void RotatingFileSink::write(const LogRecord& record) {
        rotate_if_needed();

        QTextStream stream(&file_);

        stream << format_record(record) << '\n';

        stream.flush();
    }

    QString RotatingFileSink::format_record(const LogRecord& record) {
        QString level;

        switch (record.level) {
        case LogLevel::Debug:
            level = "DEBUG";
            break;
        case LogLevel::Info:
            level = "INFO";
            break;
        case LogLevel::Warning:
            level = "WARN";
            break;
        case LogLevel::Error:
            level = "ERROR";
            break;
        case LogLevel::Fatal:
            level = "FATAL";
            break;
        }

        return QString("%1 [%2] %3")
            .arg(record.timestamp.toString("yyyy-MM-dd hh:mm:ss"))
            .arg(level)
            .arg(record.message);
    }

    void RotatingFileSink::rotate_if_needed() {
        if (file_.size() < max_size_bytes_) {
            return;
        }

        file_.close();

        auto rotating_file_path = [&](unsigned int index) -> QString {
            if (index > 0) {
                return file_path_ + "." + QString::number(index);
            }
            return file_path_;
        };

        QFile::remove(rotating_file_path(max_rotation_));
        for (unsigned int i = max_rotation_; i > 0; --i) {
            QFile::rename(rotating_file_path(i - 1), rotating_file_path(i));
        }

        file_.setFileName(file_path_);
        bool opened = file_.open(QIODevice::WriteOnly | QIODevice::Text);
    }

}
