#include "console_log_sink.hpp"
#include "log_init.hpp"
#include "log_record.hpp"
#include "logger.hpp"
#include "photo_sync.hpp"

#include <QApplication>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    auto console_sink = std::make_shared<logging::ConsoleLogSink>();
    console_sink->set_level(logging::LogLevel::Info);
    logging::Logger::instance().add_sink(console_sink);
    logging::initialize();

    PhotoSync window;
    window.add_console_log_sink(*console_sink);
    window.show();

    return QApplication::exec();
}
