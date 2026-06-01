#pragma once
#include "file_data.hpp"
#include "settings.hpp"

#include <QDir>
#include <QFileInfo>
#include <QMutex>
#include <QThread>
#include <QWaitCondition>
#include <chrono>
#include <set>
#include <unordered_map>
#include <vector>

class FileManager : public QThread {
    Q_OBJECT

public:
    FileManager(QObject* parent = nullptr);
    ~FileManager();
    FileManager(const FileManager&) = delete;
    FileManager& operator=(const FileManager&) = delete;
    FileManager(FileManager&&) noexcept = delete;
    FileManager& operator=(FileManager&&) noexcept = delete;

    void set_settings(const Settings& settings);
    bool get_status() const;

    // Qt slots
    void warning_answer(bool answer);
    void cancel();

signals:
    void warning(QString title, QString message, bool emit_answer);
    void progress_var_value(int value);
    void progress_bar_maximum(int maximum);
    void output(QString output);

private:
    static bool is_parsed(const QFileInfo& file_info, Date& date);

    void run() override;

    bool check_dirs();
    bool check_delete();
    void build_existing_file_data();
    void build_import_file_data();
    void setup_progress(int nb_import_files);
    bool already_exists(const QFileInfo& file_info);
    void export_files();
    void delete_files();
    void print_stats();
    void print_elapsed_time(std::chrono::steady_clock::time_point start,
                            std::chrono::steady_clock::time_point end);
    void add_to_progress(int val);

    static const int name_max_index;

    QStringList extensions_;

    QMutex mutex_;
    QWaitCondition condition_;
    QAtomicInt cancelled_;

    QDir import_dir_;
    QDir export_dir_;
    bool delete_files_;
    int run_count_;
    int progress_;
    int copy_progress_;
    int delete_progress_;
    std::unordered_map<qint64, std::vector<ExistingFile>> existing_files_;
    std::set<Date> directories_to_create_;
    std::vector<ExportFile> files_to_copy_;
    std::vector<QFileInfo> files_to_delete_;
    std::set<QString> import_errors_;
    std::set<QString> export_errors_;
    int duplicate_count_;
    int copy_count_;
    int delete_count_;
    bool status_;
};
