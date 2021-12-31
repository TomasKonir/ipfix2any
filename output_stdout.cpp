#include "output_stdout.h"

OutputStdout::OutputStdout(int queueLimit) : Output(queueLimit), out(stdout){
	//just for super constructor

}

void OutputStdout::next(const output_row_t row){
	out << "{";
	bool coma = false;
	foreach(const auto &f, row){
		if(!coma){
			coma = true;
		} else {
			out << ", ";
		}
		out << "\"" + f.name + "\" : " + f.value;
	}
	out << "}\n";
	out.flush();
}
