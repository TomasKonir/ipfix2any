#include <QDebug>
#include "output.h"

Output::Output(int ql) : queueLimit(ql){
	start();
}

void Output::run(){
	while(true){
		if(!queue.isEmpty()){
			output_row_t r = queue.dequeue();
			next(r);
		} else {
			mutex.lock();
			waitCondition.wait(&mutex,1000);
			mutex.unlock();
		}
		if(isInterruptionRequested()){
			break;
		}
	}
}

void Output::enqueue(const output_row_t &row){
	if(queue.count() > queueLimit){
		qInfo() << "Queue limit reached";
	}
	queue.enqueue(row);
	waitCondition.wakeOne();
}

QString Output::row2json(const output_row_t &row){
	QString ret;

	ret = "{";
	bool coma = false;
	foreach(const auto &f, row){
		if(!coma){
			coma = true;
		} else {
			ret += ", ";
		}
		ret += "\"" + f.name + "\" : " + f.value;
	}
	ret += "}";

	return(ret);
}
