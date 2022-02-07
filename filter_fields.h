#ifndef FILTERFIELDS_H
#define FILTERFIELDS_H

#include <QSet>
#include "output.h"

class FilterFields : public Filter
{
public:
	FilterFields(const QJsonObject params);
	bool next(output_row_t &row) override;

private:
	QSet<QString> pass;
	QSet<QString> remove;
};

#endif // FILTERFIELDS_H
