#include <QDebug>
#include "output.h"

static const QString charTable("abcdefghijklmnopqrstuvwxyz");

static QString nextval(QHash<QString,QString> *compressTable){
	QStringList vals = compressTable->values();
	int base = charTable.count();
	quint64 max = 0;
	foreach(QString v, vals){
		quint64 mult = 1;
		quint64 m = 0;
		foreach(QChar c, v){
			int index = charTable.indexOf(c);
			if(index >= 0){
				m += index * mult;
				mult *= base;
			}
		}
		if(m > max){
			max = m;
		}
	}

	if(vals.count()){
		max++;
	}

	QString ret("");
	do {
		ret += charTable.at(max % base);
		max /= base;
	} while(max > 0);

	return(ret);
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
	foreach(const auto &f, row.fields){
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
