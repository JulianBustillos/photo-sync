#pragma once
#include <QByteArray>
#include <QFileInfo>
#include <QString>

struct Date
{
    int m_year;
    int m_month;

    Date() : m_year(0), m_month(0) {};
    QString toQString() const;
};

bool operator<(const Date &lhs, const Date &rhs);

struct ExistingFile
{
    const QFileInfo m_info;
    QByteArray m_checksum;

    ExistingFile(const QFileInfo &info) : m_info(info) {};
};

struct ExportFile
{
    const QFileInfo m_info;
    const Date m_date;

    ExportFile(const QFileInfo &info, const Date &date) : m_info(info), m_date(date) {};
};
