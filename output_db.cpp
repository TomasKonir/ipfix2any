#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include "output_db.h"

OutputDb::OutputDb(int queueLimit, const QJsonObject &params) : Output(queueLimit) {
	foreach(QString s, QStringList({"driver","host","port","dbName","user","password","table"})){
		if(!params.contains(s)){
			qInfo() << "DB Parameter missing:" << s;
			return;
		}
	}
	this->driver = params.value("driver").toString();
	if(!db.isDriverAvailable(driver)){
		qInfo() << "Driver not available:" << driver;
		return;
	}
	this->table = params.value("table").toString();
	this->params = params;
	openDb();
}

void OutputDb::next(const output_row_t &row){
	if(!db.isOpen()){
		if(!db.isValid()){
			return;
		}
		openDb();
		if(!db.isOpen()){
			qInfo() << "Unable to open db" << params;
			return;
		}
	}
	QString r = row2json(row);
	QSqlQuery query(db);
	query.prepare("INSERT INTO " + table + "(flow) VALUES(:row)");
	query.addBindValue(r);
	if(!query.exec()){
		qInfo().noquote() << "Query exec failed for:" << r;
		qInfo() << "With error:" << query.lastError().databaseText();
	}
}

void OutputDb::openDb(){
	db = QSqlDatabase::addDatabase(driver);
	db.setDatabaseName(params.value("dbName").toString());
	db.setHostName(params.value("host").toString());
	db.setPort(params.value("port").toInt(5432));
	db.setUserName(params.value("user").toString());
	db.setPassword(params.value("password").toString());
	if(!db.open()){
		qInfo() << "Unable to open database" << params;
		return;
	}
	if(!db.tables().contains(table)){
		if(driver == "QPSQL"){
			db.exec("CREATE TABLE IF NOT EXISTS " + table + "(id BIGSERIAL PRIMARY KEY NOT NULL, tstmp TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL, flow JSONB NOT NULL)");
		} else {
			db.exec("CREATE TABLE " + table + "(flow VARCHAR NOT NULL)");
		}
		qInfo() << db.lastError().databaseText();
	}
}
