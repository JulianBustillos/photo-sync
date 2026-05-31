#include "file_data.hpp"

bool operator<(const Date& lhs, const Date& rhs) {
    if (lhs.year != rhs.year) {
        return lhs.year < rhs.year;
    }
    return lhs.month < rhs.month;
}

QString Date::to_qstring() const {
    if (year <= 0 && month <= 0) {
        return {"NO_DATE"};
    }
    return QString::number(year) + "\\" + QString::number(month).rightJustified(2, '0');
}
