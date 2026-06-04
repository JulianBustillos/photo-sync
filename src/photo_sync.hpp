#pragma once

#include "console_log_sink.hpp"
#include "file_manager.hpp"
#include "settings.hpp"
#include "ui_photo_sync.h"

#include <QLineEdit>
#include <QMainWindow>

class PhotoSync : public QMainWindow {
    Q_OBJECT

public:
    PhotoSync(QWidget* parent = Q_NULLPTR);

    void add_console_log_sink(const logging::ConsoleLogSink& sink) const;

private:
    void select_directory(const QString& title, QLineEdit& line_edit);
    void run();

    // Qt slots
    void on_start_button_clicked();
    void create_warning(const QString& title, const QString& message, bool emit_answer);
    void set_progress_bar_value(int value);
    void set_progress_bar_maximum(int maximum);
    void append_log(const logging::LogRecord& record);
    void stop();

signals:
    void warning_answer(bool answer);

private:
    Ui::PhotoSyncClass ui_;
    Settings settings_;
    FileManager file_manager_;
    bool running_;
};
