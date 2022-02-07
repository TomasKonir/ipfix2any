#include <QDebug>
#include "output_stdout.h"

OutputStdout::OutputStdout(int queueLimit, QList<Filter *> filterList, QJsonObject params) : Output(queueLimit,filterList), out(stdout){
	pretty = params.value("pretty").toBool(false);
}


void OutputStdout::next(const output_row_t &row){
	out << row2json(row,pretty);
	out << "\n";
	out.flush();
}
