#include <QDebug>
#include "output_stdout.h"

OutputStdout::OutputStdout(int queueLimit, QList<Filter *> filterList) : Output(queueLimit,filterList), out(stdout){
	//just for super constructor
}


void OutputStdout::next(const output_row_t &row){
	out << row2json(row);
	out << "\n";
	out.flush();
}
