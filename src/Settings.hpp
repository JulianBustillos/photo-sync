#pragma once
#include <QString>

class Settings {
private:
    static const QString config_filename;
    static const QString import_key;
    static const QString export_key;
    static const QString delete_key;

public:
    Settings(const QString& executable_path);

    bool parse_config_file();
    void export_config_file() const;
    void set_import_path(const QString& import_path);
    void set_export_path(const QString& export_path);
    void set_delete_files(bool delete_files);
    [[nodiscard]] QString get_import_path() const;
    [[nodiscard]] QString get_export_path() const;
    [[nodiscard]] bool get_delete_files() const;

private:
    QString config_path_;
    QString import_path_;
    QString export_path_;
    bool delete_files_;
};