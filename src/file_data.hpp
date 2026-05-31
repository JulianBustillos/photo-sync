#pragma once
#include <QByteArray>
#include <QFileInfo>
#include <QString>

struct Date {
    int year = 0;
    int month = 0;

    Date() = default;
    [[nodiscard]] QString to_qstring() const;
};

bool operator<(const Date& lhs, const Date& rhs);

struct ExistingFile {
    QFileInfo info;
    QByteArray checksum;

    ExistingFile(const QFileInfo& info) : info(info) {};
};

struct ExportFile {
    QFileInfo info;
    Date date;

    ExportFile(const QFileInfo& info, const Date& date) : info(info), date(date) {};
};
