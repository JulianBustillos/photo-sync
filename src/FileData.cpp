#include "FileData.hpp"

bool operator<(const Date &lhs, const Date &rhs)
{
    if (lhs.m_year != rhs.m_year)
        return lhs.m_year < rhs.m_year;
    return lhs.m_month < rhs.m_month;
}

QString Date::toQString() const
{
    if (m_year <= 0 && m_month <= 0)
        return QString("NO_DATE");
    return QString::number(m_year) + "\\" + QString::number(m_month).rightJustified(2, '0');
}
