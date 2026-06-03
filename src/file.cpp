#include "file.hpp"

namespace file {
    QString to_path(const QDate& date, SortMode mode) {
        QString path = "NO_DATE";
        if (date.isValid()) {
            path = QString::number(date.year());
            if (mode == SortMode::YearMonth || mode == SortMode::YearMonthDay) {
                path += "/" + QString::number(date.month()).rightJustified(2, '0');
            }
            if (mode == SortMode::YearMonthDay) {
                path += "/" + QString::number(date.day()).rightJustified(2, '0');
            }
        }
        return path;
    }
}
