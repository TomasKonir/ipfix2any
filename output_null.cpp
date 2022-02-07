#include "output_null.h"

OutputNull::OutputNull(int queueLimit, QList<Filter*> filterList) : Output(queueLimit,filterList){
	//just for super constructor
}

void OutputNull::next(const output_row_t &row) {
	Q_UNUSED(row);
}
