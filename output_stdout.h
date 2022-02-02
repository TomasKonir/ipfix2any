#ifndef OUTPUT_STDOUT_H
#define OUTPUT_STDOUT_H

#include <QTextStream>
#include <QHash>
#include "output.h"

class OutputStdout : public Output {
public:
	OutputStdout(int queueLimit);
	void next(const output_row_t &row) override;

private:
	QTextStream out;
	QHash<QString,QString> compressTable;
};

#endif
