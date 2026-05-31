#include "file_manager.hpp"

#include "date_parser.hpp"

#include <QCryptographicHash>
#include <QDirIterator>

#define MAX_NAME_INDEX 100

FileManager::FileManager(QObject* parent)
    : QThread(parent),
      extensions_({"*.jpg", "*.jpeg", "*.png", "*.mp4"}),
      cancelled_((int)false),
      status_(false),
      delete_files_(false),
      run_count_(0),
      progress_(0),
      copy_progress_(0),
      delete_progress_(0),
      duplicate_count_(0),
      copy_count_(0),
      delete_count_(0) {}

FileManager::~FileManager() {
    cancel();
    wait();
}

void FileManager::set_settings(const Settings& settings) {
    import_dir_ = QDir(settings.get_import_path());
    export_dir_ = QDir(settings.get_export_path());
    delete_files_ = settings.get_delete_files();
}

bool FileManager::get_status() const {
    return status_;
}

void FileManager::warning_answer(bool answer) {
    QMutexLocker locker(&mutex_);
    delete_files_ = answer;
    condition_.wakeAll();
}

void FileManager::cancel() {
    cancelled_.storeRelaxed((int)true);
}

bool FileManager::is_parsed(const QFileInfo& file_info, Date& date) {
    bool is_parsed = false;
    bool try_file_name = false;

    if (file_info.suffix().compare("jpg", Qt::CaseInsensitive) == 0 ||
        file_info.suffix().compare("jpeg", Qt::CaseInsensitive) == 0) {
        QFile file(file_info.absoluteFilePath());
        if (file.open(QIODevice::ReadOnly)) {
            try_file_name = !date_parser::from_jpg_buffer(file.readAll(), date);
            file.close();
            is_parsed = true;
        }
    } else if (file_info.suffix().compare("png", Qt::CaseInsensitive) == 0) {
        try_file_name = true;
        is_parsed = true;
    } else if (file_info.suffix().compare("mp4", Qt::CaseInsensitive) == 0) {
        QFile file(file_info.absoluteFilePath());
        if (file.open(QIODevice::ReadOnly)) {
            try_file_name = !date_parser::from_mp4_buffer(file.readAll(), date);
            file.close();
            is_parsed = true;
        }
    }

    if (try_file_name) {
        date_parser::from_file_name(file_info.fileName().toStdString(), date);
    }

    return is_parsed;
}

void FileManager::run() {
    cancelled_.storeRelaxed((int)false);
    progress_ = 0;
    add_to_progress(0);
    emit progress_bar_maximum(100);

    status_ = check_dir() && check_delete();

    if (status_) {
        std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();

        if ((run_count_++) > 0) {
            emit output(QString());
        }

        emit output("FROM : " + import_dir_.absolutePath());
        emit output("TO       : " + export_dir_.absolutePath());

        duplicate_count_ = 0;
        copy_count_ = 0;
        delete_count_ = 0;

        build_existing_file_data();
        build_import_file_data();
        export_files();
        delete_files();
        print_stats();

        import_errors_.clear();
        export_errors_.clear();
        existing_files_.clear();
        directories_to_create_.clear();
        files_to_copy_.clear();
        files_to_delete_.clear();

        std::chrono::steady_clock::time_point end_time = std::chrono::steady_clock::now();
        print_elapsed_time(start_time, end_time);
    }

    status_ = status_ && !(bool)cancelled_.loadRelaxed();
}

bool FileManager::check_dir() {
    if (import_dir_.isEmpty()) {
        emit warning("Path error", "Import path is empty !", false);
        return false;
    }
    if (!import_dir_.exists()) {
        emit warning("Path error",
                     "Import path : \"" + import_dir_.absolutePath() + "\" not found !",
                     false);
        return false;
    }

    if (export_dir_.isEmpty()) {
        emit warning("Path error", "Export path is empty !", false);
        return false;
    }
    if (!export_dir_.exists()) {
        emit warning("Path error",
                     "Export path : \"" + export_dir_.absolutePath() + "\" not found !",
                     false);
        return false;
    }

    return true;
}

bool FileManager::check_delete() {
    if (delete_files_) {
        QMutexLocker locker(&mutex_);
        emit warning(
            "Delete warning", "Files are going to be deleted after copy. Proceed anyway ?", true);
        condition_.wait(&mutex_);
        return delete_files_;
    }
    return true;
}

void FileManager::build_existing_file_data() {
    QDirIterator iter(
        export_dir_.absolutePath(), extensions_, QDir::Files, QDirIterator::Subdirectories);
    while (iter.hasNext()) {
        if ((bool)cancelled_.loadRelaxed()) {
            return;
        }

        QFileInfo file_info(iter.next());
        existing_files_[file_info.size()].emplace_back(file_info);
    }
}

void FileManager::build_import_file_data() {
    QDirIterator iter(
        import_dir_.absolutePath(), extensions_, QDir::Files, QDirIterator::Subdirectories);
    QVector<QFileInfo> import_file_paths;

    while (iter.hasNext()) {
        import_file_paths.append(QFileInfo(iter.next()));
    }

    int progress_size = -1;
    if (delete_files_) {
        progress_size = (int)import_file_paths.size() * 10;
        copy_progress_ = 9;
        delete_progress_ = 1;
    } else {
        progress_size = (int)import_file_paths.size();
        copy_progress_ = 1;
        delete_progress_ = 0;
    }

    if (progress_size == 0) {
        add_to_progress(100);
    } else {
        emit progress_bar_maximum(progress_size);
    }

    for (QFileInfo& file_info : import_file_paths) {
        if ((bool)cancelled_.loadRelaxed()) {
            return;
        }

        bool copy_file = true;

        auto files_iter = existing_files_.find(file_info.size());
        if (files_iter != existing_files_.end()) {
            QCryptographicHash hash(QCryptographicHash::Md5);
            QFile new_file(file_info.absoluteFilePath());
            if (!new_file.open(QIODevice::ReadOnly)) {
                import_errors_.insert(file_info.absoluteFilePath());
                add_to_progress(copy_progress_ + delete_progress_);
                continue;
            }
            hash.addData(new_file.readAll());
            QByteArray new_file_checksum = hash.result();
            new_file.close();

            for (ExistingFile& file_data : files_iter->second) {
                if (file_data.checksum.isEmpty()) {
                    QFile existing_file(file_data.info.absoluteFilePath());
                    if (!existing_file.open(QIODevice::ReadOnly)) {
                        export_errors_.insert(file_data.info.absoluteFilePath());
                        continue;
                    }

                    hash.reset();
                    hash.addData(existing_file.readAll());
                    file_data.checksum = hash.result();
                    existing_file.close();
                }

                if (new_file_checksum == file_data.checksum) {
                    copy_file = false;
                    duplicate_count_++;
                    break;
                }
            }
        }

        if (copy_file) {
            Date date;
            if (!is_parsed(file_info, date)) {
                import_errors_.insert(file_info.absoluteFilePath());
                add_to_progress(copy_progress_ + delete_progress_);
                continue;
            }
            directories_to_create_.insert(date);
            files_to_copy_.emplace_back(file_info, date);
        } else {
            if (delete_files_) {
                files_to_delete_.push_back(file_info);
            }
            add_to_progress(copy_progress_);
        }
    }
}

void FileManager::export_files() {
    for (const auto& date : directories_to_create_) {
        export_dir_.mkpath(date.to_qstring());
    }

    for (ExportFile& file_to_copy : files_to_copy_) {
        if ((bool)cancelled_.loadRelaxed()) {
            return;
        }

        QString file_name = file_to_copy.info.fileName();
        QFileInfo export_file_info(export_dir_.absolutePath() + file_to_copy.date.to_qstring() +
                                   "/" + file_name);

        int count = 0;
        bool copy = false;

        if (export_file_info.exists()) {
            int index = (int)file_name.lastIndexOf(".");
            QString name = file_name.left(index);
            QString extension = file_name.right(file_name.size() - index);
            while (export_file_info.exists() && count < MAX_NAME_INDEX) {
                export_file_info.setFile(name + "_" + QString::number(++count) + extension);
            }
        }

        if (count < MAX_NAME_INDEX) {
            QFile import_file(file_to_copy.info.absoluteFilePath());
            QFile export_file(export_file_info.absoluteFilePath());

            if (import_file.open(QIODevice::ReadOnly) && export_file.open(QIODevice::WriteOnly)) {
                copy = (export_file.write(import_file.readAll()) >= 0);
            }

            import_file.close();
            export_file.close();
        }

        if (copy) {
            copy_count_++;
            if (delete_files_) {
                files_to_delete_.push_back(file_to_copy.info);
            }
            add_to_progress(copy_progress_);
        } else {
            import_errors_.insert(file_to_copy.info.absoluteFilePath());
            add_to_progress(copy_progress_ + delete_progress_);
        }
    }
}

void FileManager::delete_files() {
    for (QFileInfo& file_to_delete : files_to_delete_) {
        QFile file(file_to_delete.absoluteFilePath());
        if (file.remove()) {
            delete_count_++;
        } else {
            import_errors_.insert(file_to_delete.absoluteFilePath());
        }

        add_to_progress(delete_progress_);
    }
}

void FileManager::print_stats() {
    if (duplicate_count_ > 0) {
        emit output(
            "Found " + QString::number(duplicate_count_) +
            (duplicate_count_ > 1 ? " already existing files." : " already existing file."));
    }

    emit output(QString::number(copy_count_) +
                (copy_count_ > 1 ? " files copied." : " file copied."));

    if (delete_count_ > 0) {
        emit output(QString::number(delete_count_) +
                    (delete_count_ > 1 ? " files deleted." : " file deleted."));
    }

    if (export_errors_.size() > 0) {
        emit output("ERROR : " + QString::number(export_errors_.size()) +
                    ((export_errors_.size() > 1)
                         ? " files in export directory could not be read !"
                         : " file in export directory could not be read !"));
    }

    if (import_errors_.size() > 0) {
        emit output("ERROR : " + QString::number(import_errors_.size()) +
                    ((import_errors_.size() > 1)
                         ? " files in import directory could not be read !"
                         : " file in import directory could not be read !"));
    }

    if ((bool)cancelled_.loadRelaxed()) {
        emit output("SYNC CANCELED !");
    }
}

void FileManager::print_elapsed_time(std::chrono::steady_clock::time_point start,
                                     std::chrono::steady_clock::time_point end) {
    int elapsed_time =
        (int)std::round(std::chrono::duration_cast<std::chrono::seconds>(end - start).count());
    int hours = 0;
    int minutes = 0;
    int seconds = 0;

    if (elapsed_time >= 3600) {
        hours = elapsed_time / 3600;
        elapsed_time -= hours * 3600;
    }

    if (elapsed_time >= 60) {
        minutes = elapsed_time / 60;
        elapsed_time -= minutes * 60;
    }

    seconds = elapsed_time;

    QString time_to_print = QString::number(hours).rightJustified(2, '0') + ":" +
                            QString::number(minutes).rightJustified(2, '0') + ":" +
                            QString::number(seconds).rightJustified(2, '0');
    emit output("Elapsed time : " + time_to_print);
}

void FileManager::add_to_progress(int val) {
    emit progress_var_value(progress_ = progress_ + val);
}
