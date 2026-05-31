#include "photo_sync.hpp"

#include <QFileDialog>
#include <QMessageBox>

PhotoSync::PhotoSync(QWidget* parent)
    : QMainWindow(parent),
      ui_(),
      settings_(QCoreApplication::applicationDirPath()),
      file_manager_(this) {
    ui_.setupUi(this);
    ui_.progressBar->setValue(0);
    ui_.startButton->setText("Start");

    if (settings_.parse_config_file()) {
        ui_.importEdit->setText(settings_.get_import_path());
        ui_.exportEdit->setText(settings_.get_export_path());
        ui_.deleteCheckBox->setChecked(settings_.get_delete_files());
    }

    QObject::connect(ui_.importToolButton, &QToolButton::clicked, this, [&]() {
        select_directory("Import directory path", *ui_.importEdit);
    });
    QObject::connect(ui_.exportToolButton, &QToolButton::clicked, this, [&]() {
        select_directory("Export directory path", *ui_.exportEdit);
    });
    QObject::connect(ui_.startButton, &QToolButton::clicked, this, &PhotoSync::run);

    QObject::connect(&file_manager_, &FileManager::warning, this, &PhotoSync::create_warning);
    QObject::connect(
        &file_manager_, &FileManager::progress_var_value, this, &PhotoSync::set_progress_bar_value);
    QObject::connect(&file_manager_,
                     &FileManager::progress_bar_maximum,
                     this,
                     &PhotoSync::set_progress_bar_maximum);
    QObject::connect(&file_manager_, &FileManager::output, this, &PhotoSync::append_output);
    QObject::connect(&file_manager_, &FileManager::finished, this, &PhotoSync::finish);
    QObject::connect(
        this, &PhotoSync::warning_answer, &file_manager_, &FileManager::warning_answer);
}

void PhotoSync::select_directory(const QString& title, QLineEdit& line_edit) {
    QString start_dir = !line_edit.text().isEmpty() ? line_edit.text() : QDir::homePath();
    QString selected_dir = QFileDialog::getExistingDirectory(this, title, start_dir);
    if (!selected_dir.isEmpty()) {
        line_edit.setText(selected_dir);
    }
}

void PhotoSync::run() {
    QObject::disconnect(ui_.startButton, nullptr, nullptr, nullptr);
    QObject::connect(ui_.startButton, &QToolButton::clicked, &file_manager_, &FileManager::cancel);
    ui_.startButton->setText("Cancel");

    settings_.set_import_path(ui_.importEdit->text());
    settings_.set_export_path(ui_.exportEdit->text());
    settings_.set_delete_files(ui_.deleteCheckBox->isChecked());

    file_manager_.set_settings(settings_);
    file_manager_.start(QThread::NormalPriority);
}

void PhotoSync::create_warning(const QString& title, const QString& message, bool emit_answer) {
    QMessageBox::StandardButton button =
        QMessageBox::warning(this,
                             title,
                             message,
                             emit_answer ? QMessageBox::Ok | QMessageBox::Cancel : QMessageBox::Ok);
    if (emit_answer) {
        emit warning_answer(button == QMessageBox::Ok);
    }
}

void PhotoSync::set_progress_bar_value(int value) {
    ui_.progressBar->setValue(value);
}

void PhotoSync::set_progress_bar_maximum(int maximum) {
    ui_.progressBar->setMaximum(maximum);
}

void PhotoSync::append_output(const QString& output) {
    ui_.textEditOutput->append(output);
}

void PhotoSync::finish() {
    QObject::disconnect(ui_.startButton, nullptr, nullptr, nullptr);
    QObject::connect(ui_.startButton, &QToolButton::clicked, this, &PhotoSync::run);
    ui_.startButton->setText("Start");

    if (file_manager_.get_status()) {
        settings_.export_config_file();
    }
}
