#include "Settings.hpp"

#include "sort_mode.hpp"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

const QString Settings::config_filename = "Configuration.json";
const QString Settings::source_key = "SourcePath";
const QString Settings::destination_key = "DestinationPath";
const QString Settings::sort_mode_key = "SortMode";
const QString Settings::remove_key = "Remove";

Settings::Settings(const QString& executable_path)
    : config_path_(executable_path + "/" + config_filename),
      source_path_(""),
      destination_path_(""),
      sort_mode_(SortMode::YearMonth),
      remove_files_(false) {}

void Settings::parse_config_file() {
    QFile config_file(config_path_);

    if (!config_file.open(QIODevice::ReadOnly)) {
        return;
    }

    QByteArray data = config_file.readAll();
    config_file.close();

    QJsonParseError parse_error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parse_error);

    if (parse_error.error != QJsonParseError::NoError) {
        qWarning() << config_path_ << " JSON parse error:" << parse_error.errorString();
        return;
    }

    if (!doc.isObject()) {
        qWarning() << config_path_ << " root is not a JSON object";
        return;
    }

    QJsonObject obj = doc.object();

    source_path_ = obj[source_key].toString();
    destination_path_ = obj[destination_key].toString();
    sort_mode_ = static_cast<SortMode>(obj[sort_mode_key].toInt());
    remove_files_ = obj[remove_key].toBool();
}

void Settings::export_config_file() const {
    QJsonObject obj;
    obj[source_key] = source_path_;
    obj[destination_key] = destination_path_;
    obj[sort_mode_key] = static_cast<int>(sort_mode_);
    obj[remove_key] = remove_files_;

    QJsonDocument doc(obj);

    QFile config_file(config_path_);

    if (!config_file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << config_path_ << " failed to open file for writing";
    }

    config_file.write(doc.toJson(QJsonDocument::Indented));

    config_file.close();
}

void Settings::set_source_path(const QString& path) {
    source_path_ = path;
}

QString Settings::get_source_path() const {
    return source_path_;
}

void Settings::set_destination_path(const QString& path) {
    destination_path_ = path;
}

QString Settings::get_destination_path() const {
    return destination_path_;
}

void Settings::set_sort_mode(SortMode mode) {
    sort_mode_ = mode;
}

SortMode Settings::get_sort_mode() const {
    return sort_mode_;
}

void Settings::set_remove_files(bool remove) {
    remove_files_ = remove;
}

bool Settings::get_remove_files() const {
    return remove_files_;
}
