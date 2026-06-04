#include "rotating_file_sink.hpp"

#include <QFileInfo>
#include <QTextStream>

namespace logging {

    RotatingFileSink::RotatingFileSink(const QString& path)
        : file_path_(std::move(path + "/PhotoSync_log.txt")), file_(file_path_) {
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

        QFile::remove(file_path_ + "." + QString::number(max_files_));

        for (int i = max_files_ - 1; i >= 1; --i) {
            QFile::rename(file_path_ + "." + QString::number(i),
                          file_path_ + "." + QString::number(i + 1));
        }

        QFile::rename(file_path_, file_path_ + ".1");

        file_.setFileName(file_path_);
        bool opened = file_.open(QIODevice::WriteOnly | QIODevice::Text);
    }

}
