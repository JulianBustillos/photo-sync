#include "console_log_sink.hpp"
#include "log_init.hpp"
#include "log_record.hpp"
#include "logger.hpp"
#include "photo_sync.hpp"
#include "rotating_file_sink.hpp"

#include <QApplication>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    auto console_sink = std::make_shared<logging::ConsoleLogSink>();
    console_sink->set_level(logging::LogLevel::Info);
    logging::Logger::instance().add_sink(console_sink);

    auto file_sink =
        std::make_shared<logging::RotatingFileSink>(QCoreApplication::applicationDirPath());
    file_sink->set_level(logging::LogLevel::Debug);
    logging::Logger::instance().add_sink(file_sink);

    logging::initialize();

    PhotoSync window;
    window.add_console_log_sink(*console_sink);
    window.show();

    return QApplication::exec();
}
