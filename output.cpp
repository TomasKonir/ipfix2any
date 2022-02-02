#include <QDebug>
#include "output.h"

static const QString charTable = "abcdefghijklmnopqrstuvwxyz";

static QString nextval(QHash<QString,QString> *compressTable){
	QString val;
	QStringList vals = compressTable->values();
	QString maxVal;
	foreach(QString v,vals){
		if(maxVal.length() < v.length() || maxVal < v){
			maxVal = v;
		}
	}
	if(maxVal.length() == 0){
		val = charTable[0];
	} else {
		QChar last = maxVal[maxVal.count() - 1];
		int pos = charTable.indexOf(last);
		if(pos < 0 || pos == (charTable.count() - 1)){
			val = maxVal + charTable[0];
		} else {
			val = maxVal;
			val[val.count() - 1] = charTable[pos+1];
		}
	}
	return(val);
}

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

QString Output::row2json(const output_row_t &row, QHash<QString,QString> *compressTable, bool autoAdd, QHash<QString, QString> *addedKeys){
	QString ret;

	ret = "{";
	bool coma = false;
	foreach(const auto &f, row){
		if(!coma){
			coma = true;
		} else {
			ret += ", ";
		}
		if(compressTable != nullptr){
			if(compressTable->contains(f.name)){
				ret += "\"" + compressTable->value(f.name) + "\" : " + f.value;
			} else if(autoAdd){
				QString val = nextval(compressTable);
				qInfo() << "Added" << f.name << val;
				compressTable->insert(f.name,val);
				if(addedKeys){
					addedKeys->insert(f.name,val);
				}
				ret += "\"" + compressTable->value(f.name) + "\" : " + f.value;
			} else {
				ret += "\"" + f.name + "\" : " + f.value;
			}
		} else {
			ret += "\"" + f.name + "\" : " + f.value;
		}
	}
	ret += "}";

	return(ret);
}
