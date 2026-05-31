#include "Settings.hpp"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

const QString Settings::config_filename = "Configuration.json";
const QString Settings::import_key = "ImportPath";
const QString Settings::export_key = "ExportPath";
const QString Settings::delete_key = "Delete";

Settings::Settings(const QString& executable_path)
    : config_path_(executable_path + "/" + config_filename),
      import_path_(""),
      export_path_(""),
      delete_files_(false) {}

bool Settings::parse_config_file() {
    QFile config_file(config_path_);

    if (!config_file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QByteArray data = config_file.readAll();
    config_file.close();

    QJsonParseError parse_error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parse_error);

    if (parse_error.error != QJsonParseError::NoError) {
        qWarning() << config_path_ << " JSON parse error:" << parse_error.errorString();
        return false;
    }

    if (!doc.isObject()) {
        qWarning() << config_path_ << " root is not a JSON object";
        return false;
    }

    QJsonObject obj = doc.object();

    import_path_ = obj[import_key].toString();
    export_path_ = obj[export_key].toString();
    delete_files_ = obj[delete_key].toBool();

    return true;
}

void Settings::export_config_file() const {
    QJsonObject obj;
    obj[import_key] = import_path_;
    obj[export_key] = export_path_;
    obj[delete_key] = delete_files_;

    QJsonDocument doc(obj);

    QFile config_file(config_path_);

    if (!config_file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << config_path_ << " failed to open file for writing";
    }

    config_file.write(doc.toJson(QJsonDocument::Indented));

    config_file.close();
}

void Settings::set_import_path(const QString& import_path) {
    import_path_ = import_path;
}

void Settings::set_export_path(const QString& export_path) {
    export_path_ = export_path;
}

void Settings::set_delete_files(bool delete_files) {
    delete_files_ = delete_files;
}

QString Settings::get_import_path() const {
    return import_path_;
}

QString Settings::get_export_path() const {
    return export_path_;
}

bool Settings::get_delete_files() const {
    return delete_files_;
}
