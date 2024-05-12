#ifndef FILTERZERO_H
#define FILTERZERO_H

#include <QSet>
#include "output.h"

class FilterZero : public Filter
{
public:
    FilterZero(const QJsonObject params);
    bool next(output_row_t &row) override;
};

#endif  // FILTERZERO_H
