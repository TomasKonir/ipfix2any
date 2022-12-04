#include <QDebug>
#include "output.h"

static const QString charTable("abcdefghijklmnopqrstuvwxyz");

static QString nextval(QHash<QString,QString> *compressTable){    
    QStringList vals = compressTable->values();
    std::sort(vals.begin(), vals.end(),[](const QString &a, QString &b){
        if(a.count() != b.count()){
            return(a.count() < b.count());
        } else {
            return(a < b);
        }
    });
    if(vals.count() == 0){
        return(charTable[0]);
    }
    QString ret = vals.last();

    for(int i = (ret.count() - 1); i >= 0;i--){
        QChar ch    = ret[i];
        int   index = charTable.indexOf(ch) + 1;
        if(index < charTable.count()){
            ret[i] = charTable[index];
            break;
        } else {
            ret[i] = charTable[0];
            if(i == 0){
                ret = charTable[0] + ret;
            }
        }
    }

    return(ret);
}

Filter::Filter(const QJsonObject params){
	Q_UNUSED(params);
}

Output::Output(int ql, QList<Filter *> filterList) : queueLimit(ql){
	this->filterList = filterList;
	start();
}

Output::~Output(){
	foreach(Filter *f, filterList){
		delete(f);
	}
	filterList.clear();
}

void Output::run(){
	int filterCount = filterList.count();
	while(true){
		mutex.lock();
		if(!queue.isEmpty()){
			output_row_t r = queue.dequeue();
			mutex.unlock();
			bool ready2process = true;
			if(filterCount){
				foreach(Filter *f, filterList){
					if(!f->next(r)){
						break;
					}
				}
			}
			if(ready2process){
				next(r);
			}
		} else {
			waitCondition.wait(&mutex,1000);
			mutex.unlock();
		}
		if(isInterruptionRequested()){
			break;
		}
	}
}

void Output::enqueue(const output_row_t &row){
	mutex.lock();
	if(queue.count() > queueLimit){
		qInfo() << "Queue limit reached";
	}
	queue.enqueue(row);
	waitCondition.wakeOne();
	mutex.unlock();
}

QString Output::row2json(const output_row_t &row, bool pretty, QHash<QString,QString> *compressTable, bool autoAdd, QHash<QString, QString> *addedKeys){
	QString ret;

	ret = "{";
	bool coma = false;
	foreach(const auto &f, row.fields){
		if(!coma){
			coma = true;
		} else {
			ret += ", ";
		}
		if(pretty){
			ret += "\n\t";
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
	if(pretty){
		ret += "\n";
	}
	ret += "}";

	return(ret);
}
