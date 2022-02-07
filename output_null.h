#ifndef OUTPUT_NULL_H
#define OUTPUT_NULL_H

#include "output.h"

class OutputNull : public Output {
public:
	OutputNull(int queueLimit, QList<Filter*> filterList);
	void next(const output_row_t &row) override;
};

#endif
