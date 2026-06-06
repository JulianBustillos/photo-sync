#include "file_manager.hpp"

#include "date_parser.hpp"
#include "file.hpp"
#include "sort_mode.hpp"

#include <QCryptographicHash>
#include <QDirIterator>
#include <qlogging.h>

const int FileManager::name_max_index = 100;

FileManager::FileManager(QObject* parent)
    : QThread(parent),
      waiting_(false),
      answer_(false),
      cancelled_(static_cast<int>(false)),
      extensions_({"*.jpg", "*.jpeg", "*.png", "*.mp4"}),
      status_(false),
      sort_mode_(SortMode::YearMonth),
      remove_files_(false),
      progress_(0) {}

FileManager::~FileManager() {
    cancel();
    wait();
}

void FileManager::set_settings(const Settings& settings) {
    source_dir_ = QDir(settings.get_source_path());
    destination_dir_ = QDir(settings.get_destination_path());
    sort_mode_ = settings.get_sort_mode();
    remove_files_ = settings.get_remove_files();
}

void FileManager::cancel() {
    cancelled_.storeRelaxed(static_cast<int>(true));
}

bool FileManager::get_status() const {
    return status_;
}

void FileManager::set_warning_answer(bool accepted) {
    QMutexLocker locker(&mutex_);
    answer_ = accepted;
    waiting_ = false;
    wait_condition_.wakeOne();
}

FileManager::Context::Context(SortMode mode)
    : copy_progress(0),
      remove_progress(0),
      duplicate_count(0),
      copy_count(0),
      remove_count(0),
      directories_to_create([mode](const QDate& lhs, const QDate& rhs) -> bool {
          if (mode == SortMode::Year || lhs.year() != rhs.year()) {
              return lhs.year() < rhs.year();
          }
          if (mode == SortMode::YearMonth || lhs.month() != rhs.month()) {
              return lhs.month() < rhs.month();
          }
          return lhs.day() < rhs.day();
      }) {}

bool FileManager::parse_date(const QFileInfo& file_info, QDate& date) {
    date = QDate();
    bool is_valid = false;

    if (file_info.suffix().compare("jpg", Qt::CaseInsensitive) == 0 ||
        file_info.suffix().compare("jpeg", Qt::CaseInsensitive) == 0) {
        QFile file(file_info.absoluteFilePath());
        if (file.open(QIODevice::ReadOnly)) {
            date = date_parser::from_jpg_buffer(file.readAll());
            file.close();
            is_valid = true;
        }
    } else if (file_info.suffix().compare("png", Qt::CaseInsensitive) == 0) {
        is_valid = true;
    } else if (file_info.suffix().compare("mp4", Qt::CaseInsensitive) == 0) {
        QFile file(file_info.absoluteFilePath());
        if (file.open(QIODevice::ReadOnly)) {
            date = date_parser::from_mp4_buffer(file.readAll());
            file.close();
            is_valid = true;
        }
    }

    if (!date.isValid()) {
        date = date_parser::from_file_name(file_info.fileName().toStdString());
    }

    return is_valid;
}

void FileManager::log_elapsed_time(std::chrono::steady_clock::time_point start,
                                   std::chrono::steady_clock::time_point end) {
    int elapsed_time = static_cast<int>(
        std::round(std::chrono::duration_cast<std::chrono::seconds>(end - start).count()));
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

    qInfo().noquote() << "Elapsed time : "
                      << QString::fromStdString(
                             std::format("{:02}:{:02}:{:02}", hours, minutes, seconds));
}

void FileManager::run() {
    cancelled_.storeRelaxed(static_cast<int>(false));
    progress_ = 0;
    add_to_progress(0);
    emit progress_bar_maximum(100);

    status_ = check_dirs() && check_remove();

    if (status_) {
        std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();

        qInfo() << "Photos to organize: " << source_dir_.absolutePath();
        qInfo() << "Destination folder: " << destination_dir_.absolutePath();

        Context context(sort_mode_);

        collect_destination_fingerprints(context);
        collect_source_entries(context);
        organize_files(context);
        remove_files(context);
        log_stats(context);

        std::chrono::steady_clock::time_point end_time = std::chrono::steady_clock::now();
        log_elapsed_time(start_time, end_time);
    }

    status_ = status_ && !static_cast<bool>(cancelled_.loadRelaxed());
}

bool FileManager::check_dirs() {
    if (!source_dir_.exists()) {
        emit warning("Path error",
                     "source path : \"" + source_dir_.absolutePath() + "\" not found !",
                     false);
        return false;
    }

    if (source_dir_.isEmpty()) {
        emit warning("Path error", "source path is empty !", false);
        return false;
    }

    if (!destination_dir_.exists()) {
        emit warning("Path error",
                     "destination path : \"" + destination_dir_.absolutePath() + "\" not found !",
                     false);
        return false;
    }

    return true;
}

bool FileManager::check_remove() {
    if (remove_files_) {
        QMutexLocker locker(&mutex_);
        waiting_ = true;
        emit warning(
            "Remove warning", "Files are going to be removed after copy. Proceed anyway ?", true);

        while (waiting_) {
            wait_condition_.wait(&mutex_);
        }

        remove_files_ = answer_;
        return remove_files_;
    }
    return true;
}

void FileManager::collect_destination_fingerprints(Context& context) {
    QDirIterator iter(
        destination_dir_.absolutePath(), extensions_, QDir::Files, QDirIterator::Subdirectories);
    while (iter.hasNext()) {
        if (static_cast<bool>(cancelled_.loadRelaxed())) {
            return;
        }

        QFileInfo file_info(iter.next());
        context.destination_fingerprints[file_info.size()].emplace_back(file_info);
    }
}

void FileManager::collect_source_entries(Context& context) {
    QDirIterator iter(
        source_dir_.absolutePath(), extensions_, QDir::Files, QDirIterator::Subdirectories);
    QVector<QFileInfo> source_file_paths;

    while (iter.hasNext()) {
        source_file_paths.append(QFileInfo(iter.next()));
    }

    setup_progress(context, static_cast<int>((source_file_paths.size())));

    for (const QFileInfo& file_info : source_file_paths) {
        if (static_cast<bool>(cancelled_.loadRelaxed())) {
            return;
        }

        if (!already_exists(context, file_info)) {
            QDate date;
            if (!parse_date(file_info, date)) {
                context.source_errors.insert(file_info.absoluteFilePath());
                add_to_progress(context.copy_progress + context.remove_progress);
                continue;
            }
            context.directories_to_create.insert(date);
            context.files_to_copy.emplace_back(file_info, date);
        } else {
            if (remove_files_) {
                context.files_to_remove.push_back(file_info);
            }
            add_to_progress(context.copy_progress);
        }
    }
}

void FileManager::setup_progress(Context& context, int nb_source_files) {
    int progress_size = -1;
    if (remove_files_) {
        progress_size = nb_source_files * 10;
        context.copy_progress = 9;
        context.remove_progress = 1;
    } else {
        progress_size = nb_source_files;
        context.copy_progress = 1;
        context.remove_progress = 0;
    }

    if (progress_size == 0) {
        add_to_progress(100);
    } else {
        emit progress_bar_maximum(progress_size);
    }
}

bool FileManager::already_exists(Context& context, const QFileInfo& file_info) {
    bool same_file_found = false;

    auto fingerprints_iter = context.destination_fingerprints.find(file_info.size());
    if (fingerprints_iter != context.destination_fingerprints.end()) {
        QCryptographicHash hash(QCryptographicHash::Md5);
        QFile new_file(file_info.absoluteFilePath());
        if (!new_file.open(QIODevice::ReadOnly)) {
            context.source_errors.insert(file_info.absoluteFilePath());
            add_to_progress(context.copy_progress + context.remove_progress);
            same_file_found = true;
        } else {
            hash.addData(new_file.readAll());
            QByteArray new_file_checksum = hash.result();
            new_file.close();

            for (file::Fingerprint& fingerprints : fingerprints_iter->second) {
                if (fingerprints.checksum.isEmpty()) {
                    QFile existing_file(fingerprints.info.absoluteFilePath());
                    if (!existing_file.open(QIODevice::ReadOnly)) {
                        context.destination_errors.insert(fingerprints.info.absoluteFilePath());
                        continue;
                    }

                    hash.reset();
                    hash.addData(existing_file.readAll());
                    fingerprints.checksum = hash.result();
                    existing_file.close();
                }

                if (new_file_checksum == fingerprints.checksum) {
                    same_file_found = true;
                    context.duplicate_count++;
                    break;
                }
            }
        }
    }

    return same_file_found;
}

void FileManager::organize_files(Context& context) {
    for (const auto& date : context.directories_to_create) {
        destination_dir_.mkpath(file::to_path(date, sort_mode_));
    }

    for (file::Entry& file_to_copy : context.files_to_copy) {
        if (static_cast<bool>(cancelled_.loadRelaxed())) {
            return;
        }

        QString file_name = file_to_copy.info.fileName();
        QFileInfo destination_file_info(destination_dir_.filePath(
            file::to_path(file_to_copy.date, sort_mode_) + "/" + file_name));

        int count = 0;
        bool copy = false;

        if (destination_file_info.exists()) {
            int index = static_cast<int>(file_name.lastIndexOf("."));
            QString name = file_name.left(index);
            QString extension = file_name.right(file_name.size() - index);
            while (destination_file_info.exists() && count < name_max_index) {
                destination_file_info.setFile(name + "_" + QString::number(++count) + extension);
            }
        }

        if (count < name_max_index) {
            QFile source_file(file_to_copy.info.absoluteFilePath());
            QFile destination_file(destination_file_info.absoluteFilePath());

            if (source_file.open(QIODevice::ReadOnly) &&
                destination_file.open(QIODevice::WriteOnly)) {
                copy = (destination_file.write(source_file.readAll()) >= 0);
            }

            source_file.close();
            destination_file.close();
        }

        if (copy) {
            context.copy_count++;
            if (remove_files_) {
                context.files_to_remove.push_back(file_to_copy.info);
            }
            add_to_progress(context.copy_progress);
        } else {
            context.source_errors.insert(file_to_copy.info.absoluteFilePath());
            add_to_progress(context.copy_progress + context.remove_progress);
        }
    }
}

void FileManager::remove_files(Context& context) {
    for (QFileInfo& file_to_delete : context.files_to_remove) {
        QFile file(file_to_delete.absoluteFilePath());
        if (file.remove()) {
            context.remove_count++;
        } else {
            context.source_errors.insert(file_to_delete.absoluteFilePath());
        }

        add_to_progress(context.remove_progress);
    }
}

void FileManager::log_stats(Context& context) {
    if (context.duplicate_count > 0) {
        qInfo() << "Found " << context.duplicate_count
                << (context.duplicate_count > 1 ? " already existing files."
                                                : " already existing file.");
    }

    qInfo() << context.copy_count
            << (context.copy_count > 1 ? " files organized." : " file organized.");

    if (context.remove_count > 0) {
        qInfo() << context.remove_count
                << (context.remove_count > 1 ? " files removed." : " file removed.");
    }

    if (context.destination_errors.size() > 0) {
        qInfo() << "ERROR : " << context.destination_errors.size()
                << ((context.destination_errors.size() > 1)
                        ? " files in destination directory could not be read !"
                        : " file in destination directory could not be read !");
    }

    if (context.source_errors.size() > 0) {
        qInfo() << "ERROR : " << context.source_errors.size()
                << ((context.source_errors.size() > 1)
                        ? " files in source directory could not be read !"
                        : " file in source directory could not be read !");
    }

    if (static_cast<bool>(cancelled_.loadRelaxed())) {
        qInfo() << "Sync canceled !";
    }
}

void FileManager::add_to_progress(int val) {
    emit progress_var_value(progress_ = progress_ + val);
}
