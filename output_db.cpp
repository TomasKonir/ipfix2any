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
	this->lastCommit = QDateTime::currentDateTime();
	this->table = params.value("table").toString();
	this->params = params;
	this->compressKeys = params.value("compressKeys").toBool(false);
	openDb();
}

OutputDb::~OutputDb(){
	if(db.isOpen()){
		db.commit();
	}
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
	QString r;
	if(compressKeys){
		QHash<QString,QString> added;
		r = row2json(row,&compressTable,true,&added);
		if(added.count()){
			//force end transaction
			db.commit();
			lastCommit = QDateTime::currentDateTime();
			foreach(QString k, added.keys()){
				QSqlQuery aq(db);
				aq.prepare("INSERT INTO " + table + "_keys(name,value) VALUES(:name,:value)");
				aq.addBindValue(k);
				aq.addBindValue(added.value(k));
				if(!aq.exec()){
					qInfo().noquote() << "Query exec failed for:" << k << added.value(k);
					qInfo() << "With error:" << aq.lastError().databaseText();
				}
			}
		}
	} else {
		r = row2json(row);
	}
	QSqlQuery query(db);
	query.prepare("INSERT INTO " + table + "(flow) VALUES(:row)");
	query.addBindValue(r);
	if(!query.exec()){
		qInfo().noquote() << "Query exec failed for:" << r;
		qInfo() << "With error:" << query.lastError().databaseText();
	} else {
		if(qAbs(lastCommit.secsTo(QDateTime::currentDateTime())) > 10){
			db.commit();
			db.transaction();
			lastCommit = QDateTime::currentDateTime();
		}
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
	if(!db.tables().contains(table) || !db.tables().contains(table + "_keys")){
		if(driver == "QPSQL"){
			db.exec("CREATE TABLE IF NOT EXISTS " + table + "(id BIGSERIAL PRIMARY KEY NOT NULL, tstmp TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL, flow JSONB NOT NULL)");
			if(compressKeys){
				db.exec("CREATE TABLE IF NOT EXISTS " + table + "_keys(name VARCHAR NOT NULL UNIQUE, value VARCHAR NOT NULL UNIQUE)");
			}
		} else {
			db.exec("CREATE TABLE " + table + "(flow VARCHAR NOT NULL)");
			if(compressKeys){
				db.exec("CREATE TABLE " + table + "_keys(name VARCHAR NOT NULL, value VARCHAR NOT NULL)");
			}
		}
		if(db.lastError().type() != QSqlError::NoError){
			qInfo() << db.lastError().databaseText();
		}
	}
	if(compressKeys){
		qInfo() << "Loading compress table";
		compressTable.clear();
		QSqlQuery q = db.exec("SELECT name,value FROM " + table + "_keys");
		while(q.next()){
			compressTable.insert(q.value(0).toString(),q.value(1).toString());
			qInfo() << q.value(0).toString() << q.value(1).toString();
		}
		qInfo() << "Compress table loaded";
	}
	//begin transaction
	db.transaction();
}
