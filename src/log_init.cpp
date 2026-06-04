#include "log_init.hpp"

#include "log_record.hpp"
#include "logger.hpp"

#include <QDateTime>
#include <QtGlobal>

namespace logging {

    namespace {

        LogLevel to_log_level(QtMsgType type) {
            switch (type) {
            case QtDebugMsg:
                return LogLevel::Debug;

            case QtInfoMsg:
                return LogLevel::Info;

            case QtWarningMsg:
                return LogLevel::Warning;

            case QtCriticalMsg:
                return LogLevel::Error;

            case QtFatalMsg:
                return LogLevel::Fatal;
            }

            return LogLevel::Info;
        }

        void qt_message_handler(QtMsgType type,
                                const QMessageLogContext& context,
                                const QString& message) {
            LogRecord record;

            record.timestamp = QDateTime::currentDateTime();
            record.level = to_log_level(type);
            record.category = context.category;
            record.message = message;

            Logger::instance().log(record);

            if (type == QtFatalMsg) {
                abort();
            }
        }

    }

    void initialize() {
        qInstallMessageHandler(qt_message_handler);
    }

}