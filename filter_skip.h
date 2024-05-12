#ifndef FILTERSKIP_H
#define FILTERSKIP_H

#include <QSet>
#include "output.h"

class FilterSkip : public Filter
{
public:
    FilterSkip(const QJsonObject params);
    bool next(output_row_t &row) override;

private:
	QSet<QString> skip;
};

#endif  // FILTERSKIP_H
