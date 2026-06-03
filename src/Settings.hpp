#pragma once

#include "sort_mode.hpp"

#include <QString>

class Settings {
private:
    static const QString config_filename;
    static const QString source_key;
    static const QString destination_key;
    static const QString sort_mode_key;
    static const QString remove_key;

public:
    Settings(const QString& executable_path);

    void parse_config_file();
    void export_config_file() const;
    void set_source_path(const QString& path);
    [[nodiscard]] QString get_source_path() const;
    void set_destination_path(const QString& path);
    [[nodiscard]] QString get_destination_path() const;
    void set_sort_mode(SortMode mode);
    [[nodiscard]] SortMode get_sort_mode() const;
    void set_remove_files(bool remove);
    [[nodiscard]] bool get_remove_files() const;

private:
    QString config_path_;
    QString source_path_;
    QString destination_path_;
    SortMode sort_mode_;
    bool remove_files_;
};