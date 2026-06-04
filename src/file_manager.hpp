#pragma once

#include "file.hpp"
#include "settings.hpp"
#include "sort_mode.hpp"

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
    void cancel();
    bool get_status() const;

    // Qt slots
    void warning_answer(bool answer);

signals:
    void warning(QString title, QString message, bool emit_answer);
    void progress_var_value(int value);
    void progress_bar_maximum(int maximum);

private:
    struct Context {
        Context(SortMode mode);

        int copy_progress;
        int remove_progress;
        int duplicate_count;
        int copy_count;
        int remove_count;
        std::unordered_map<qint64, std::vector<file::Fingerprint>> destination_fingerprints;
        std::set<QDate, std::function<bool(const QDate&, const QDate&)>> directories_to_create;
        std::vector<file::Entry> files_to_copy;
        std::vector<QFileInfo> files_to_remove;
        std::set<QString> source_errors;
        std::set<QString> destination_errors;
    };

    static bool parse_date(const QFileInfo& file_info, QDate& date);
    static void log_elapsed_time(std::chrono::steady_clock::time_point start,
                                 std::chrono::steady_clock::time_point end);

    static const int name_max_index;

    void run() override;

    bool check_dirs();
    bool check_remove();
    void collect_destination_fingerprints(Context& context);
    void collect_source_entries(Context& context);
    void setup_progress(Context& context, int nb_source_files);
    bool already_exists(Context& context, const QFileInfo& file_info);
    void organize_files(Context& context);
    void remove_files(Context& context);
    void log_stats(Context& context);
    void add_to_progress(int val);

    QMutex mutex_;
    QWaitCondition condition_;
    QAtomicInt cancelled_;

    QStringList extensions_;

    QDir source_dir_;
    QDir destination_dir_;
    SortMode sort_mode_;
    bool remove_files_;
    int progress_;
    bool status_;
};
