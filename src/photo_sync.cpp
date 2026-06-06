#include "photo_sync.hpp"

#include "console_log_sink.hpp"

#include <QFileDialog>
#include <QMessageBox>

PhotoSync::PhotoSync(QWidget* parent)
    : QMainWindow(parent),
      ui_(),
      settings_(QCoreApplication::applicationDirPath()),
      file_manager_(this),
      running_(false) {
    ui_.setupUi(this);
    ui_.sort_mode_combo_box->addItem("Year", static_cast<int>(SortMode::Year));
    ui_.sort_mode_combo_box->addItem("Year/Month", static_cast<int>(SortMode::YearMonth));
    ui_.sort_mode_combo_box->addItem("Year/Month/Day", static_cast<int>(SortMode::YearMonthDay));
    ui_.progress_bar->setValue(0);
    ui_.start_button->setText("Start");

    settings_.parse_config_file();
    ui_.source_edit->setText(settings_.get_source_path());
    ui_.destination_edit->setText(settings_.get_destination_path());
    int mode_index = ui_.sort_mode_combo_box->findData(static_cast<int>(settings_.get_sort_mode()));
    ui_.sort_mode_combo_box->setCurrentIndex(mode_index);
    ui_.remove_check_box->setChecked(settings_.get_remove_files());

    QObject::connect(ui_.source_tool_button, &QToolButton::clicked, this, [&]() {
        select_directory("Select source folder", *ui_.source_edit);
    });
    QObject::connect(ui_.destination_tool_button, &QToolButton::clicked, this, [&]() {
        select_directory("Select destination folder", *ui_.destination_edit);
    });
    QObject::connect(
        ui_.start_button, &QToolButton::clicked, this, &PhotoSync::on_start_button_clicked);

    QObject::connect(&file_manager_, &FileManager::warning, this, &PhotoSync::create_warning);
    QObject::connect(
        &file_manager_, &FileManager::progress_var_value, this, &PhotoSync::set_progress_bar_value);
    QObject::connect(&file_manager_,
                     &FileManager::progress_bar_maximum,
                     this,
                     &PhotoSync::set_progress_bar_maximum);
    QObject::connect(&file_manager_, &FileManager::finished, this, &PhotoSync::stop);
    QObject::connect(
        this, &PhotoSync::warning_answer, &file_manager_, &FileManager::set_warning_answer);
}

void PhotoSync::add_console_log_sink(const logging::ConsoleLogSink& sink) const {
    QObject::connect(&sink, &logging::ConsoleLogSink::log_received, this, &PhotoSync::append_log);
}

void PhotoSync::select_directory(const QString& title, QLineEdit& line_edit) {
    QString start_dir = !line_edit.text().isEmpty() ? line_edit.text() : QDir::homePath();
    QString selected_dir = QFileDialog::getExistingDirectory(this, title, start_dir);
    if (!selected_dir.isEmpty()) {
        line_edit.setText(selected_dir);
    }
}

void PhotoSync::run() {
    running_ = true;
    ui_.start_button->setText("Cancel");

    settings_.set_source_path(ui_.source_edit->text());
    settings_.set_destination_path(ui_.destination_edit->text());
    settings_.set_sort_mode(static_cast<SortMode>(ui_.sort_mode_combo_box->currentData().toInt()));
    settings_.set_remove_files(ui_.remove_check_box->isChecked());

    file_manager_.set_settings(settings_);
    file_manager_.start(QThread::NormalPriority);
}

void PhotoSync::on_start_button_clicked() {
    if (!running_) {
        run();
    } else {
        file_manager_.cancel();
    }
}

void PhotoSync::create_warning(const QString& title, const QString& message, bool emit_answer) {
    QMessageBox::StandardButton button =
        QMessageBox::warning(this,
                             title,
                             message,
                             wait_answer ? QMessageBox::Ok | QMessageBox::Cancel : QMessageBox::Ok);
    if (wait_answer) {
        emit warning_answer(button == QMessageBox::Ok);
    }
}

void PhotoSync::set_progress_bar_value(int value) {
    ui_.progress_bar->setValue(value);
}

void PhotoSync::set_progress_bar_maximum(int maximum) {
    ui_.progress_bar->setMaximum(maximum);
}

void PhotoSync::append_log(const logging::LogRecord& record) {
    ui_.log_text_edit->append(record.message);
}

void PhotoSync::stop() {
    running_ = false;
    ui_.start_button->setText("Start");

    if (file_manager_.get_status()) {
        settings_.export_config_file();
    }
}
