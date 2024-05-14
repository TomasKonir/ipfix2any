#include "output_db.h"
#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>

OutputDb::OutputDb(int queueLimit, QList<Filter*> filterList, const QJsonObject& params) : Output(queueLimit, filterList) {
    foreach (QString s, QStringList({"driver", "host", "port", "dbName", "user", "password", "table"})) {
        if (!params.contains(s)) {
            qInfo() << "DB Parameter missing:" << s;
            return;
        }
    }
    this->driver = params.value("driver").toString();
    if (!db.isDriverAvailable(driver)) {
        qInfo() << "Driver not available:" << driver;
        return;
    }
    this->lastCommit = QDateTime::currentDateTime();
    this->table = params.value("table").toString();
    this->params = params;
    this->compressKeys = params.value("compressKeys").toBool(false);
    this->partitioning = params.value("partitioning").toBool(false);
    this->keepPartitions = params.value("keepPartitions").toInt();
    if (params.contains("optimizedTables")) {
        this->optimizedTableFields = params.value("optimizedTables").toObject();
    }
    if (driver != "QPSQL" && this->partitioning) {
        qInfo() << "Partitioning is available for PSQL driver only";
        this->partitioning = false;
    }
    openDb();
}

OutputDb::~OutputDb() {
    if (db.isOpen()) {
        db.commit();
    }
}

void OutputDb::next(const output_row_t& row) {
    if (!db.isOpen()) {
        if (!db.isValid()) {
            return;
        }
        openDb();
        if (!db.isOpen()) {
            qInfo() << "Unable to open db" << params;
            return;
        }
    }
    QString r;
    output_row_t rowCopy;
    QDateTime now = QDateTime::currentDateTimeUtc();
    QString tableName = table;
    QJsonObject optimizedFields;
    if (optimizedTableFields.count()) {
        QStringList keys = optimizedTableFields.keys();
        rowCopy.templateId = row.templateId;
        for (const output_field_t& field : row.fields) {
            bool found = false;
            if (keys.contains(field.name)) {
                bool ok;
                field.value.toLongLong(&ok);
                if (ok) {
                    optimizedFields.insert(field.name, field.value.toLongLong());
                } else {
                    optimizedFields.insert(field.name, field.value);
                }
                found = true;
            } else if (optimizedTableWildcards.count()) {
                for (const QString& s : optimizedTableWildcards) {
                    if (field.name.startsWith(s)) {
                        bool ok;
                        field.value.toLongLong(&ok);
                        if (ok) {
                            optimizedFields.insert(s, field.value.toLongLong());
                        } else {
                            optimizedFields.insert(s, field.value);
                        }
                        found = true;
                        break;
                    }
                }
            }
            if (!found) {
                rowCopy.fields << field;
            }
        }
    }
    if (compressKeys) {
        QHash<QString, QString> added;
        if (rowCopy.fields.count()) {
            r = row2json(rowCopy, false, &compressTable, true, &added);
        } else {
            r = row2json(row, false, &compressTable, true, &added);
        }
        if (added.count()) {
            // force end transaction
            db.commit();
            lastCommit = QDateTime::currentDateTime();
            foreach (QString k, added.keys()) {
                QSqlQuery aq(db);
                aq.prepare("INSERT INTO " + table + "_keys(name,value) VALUES(:name,:value)");
                aq.addBindValue(k);
                aq.addBindValue(added.value(k));
                if (!aq.exec()) {
                    qInfo().noquote() << "Query exec failed for:" << k << added.value(k);
                    qInfo() << "With error:" << aq.lastError().databaseText();
                }
            }
        }
    } else {
        if (rowCopy.fields.count()) {
            r = row2json(rowCopy);
        } else {
            r = row2json(row);
        }
    }
    if (partitioning) {
        tableName += now.toString("_yyyyMM");
        if (!db.tables().contains(tableName)) {
            int year = now.date().year();
            int month = now.date().month();
            month++;
            if (month > 12) {
                month = 1;
                year++;
            }
            QString from = now.toString("yyyy-MM-01");
            QString to = QString("%1-%2-01").arg(year, 4, 10, QChar('0')).arg(month, 2, 10, QChar('0'));
            QString createTable = "CREATE TABLE " + tableName + " PARTITION OF " + table + " FOR VALUES FROM ('" + from + "') TO ('" + to + "')";
            db.exec(createTable);
            if (db.lastError().type() != QSqlError::NoError) {
                qInfo() << db.lastError().databaseText();
                return;
            }
            qInfo() << "Creating table partition: " << tableName << createTable;
            if (keepPartitions > 0) {
                QStringList tables;
                foreach (QString t, db.tables()) {
                    if (t.startsWith(table + "_") && t != (table + "_keys")) {
                        tables << t;
                    }
                }
                tables.sort();
                for (int i = 0; i < keepPartitions; i++) {
                    if (tables.count() > 0) {
                        tables.removeLast();
                    }
                }
                foreach (QString t, tables) {
                    db.exec("DROP TABLE " + t);
                }
            }
        }
    }
    QSqlQuery query(db);
    if (optimizedFields.count()) {
        QStringList insertKeys;
        for (const QString& k : optimizedFields.keys()) {
            insertKeys << (":" + k);
        }
        query.prepare("INSERT INTO " + tableName + "(" + optimizedFields.keys().join(",") + ",tstmp,flow) VALUES(" + insertKeys.join(",") + ",:tstmp,:row)");
        for (const QString& k : optimizedFields.keys()) {
            if (optimizedFields.value(k).isString()) {
                query.addBindValue(optimizedFields.value(k).toString().remove("\""));
            } else {
                query.addBindValue(optimizedFields.value(k).toDouble());
            }
        }
    } else {
        query.prepare("INSERT INTO " + tableName + "(tstmp,flow) VALUES(:tstmp,:row)");
    }

    query.addBindValue(now);
    query.addBindValue(r);
    if (!query.exec()) {
        qInfo().noquote() << "Query exec failed for:" << r;
        qInfo().noquote() << "Query:" << query.lastQuery();
        qInfo().noquote() << "Error:" << query.lastError().databaseText();
    } else {
        if (qAbs(lastCommit.secsTo(QDateTime::currentDateTime())) > 10) {
            db.commit();
            db.transaction();
            lastCommit = QDateTime::currentDateTime();
        }
    }
}

void OutputDb::openDb() {
    QStringList tableFields;
    db = QSqlDatabase::addDatabase(driver);
    db.setDatabaseName(params.value("dbName").toString());
    db.setHostName(params.value("host").toString());
    db.setPort(params.value("port").toInt(5432));
    db.setUserName(params.value("user").toString());
    db.setPassword(params.value("password").toString());
    if (!db.open()) {
        qInfo() << "Unable to open database" << params;
        return;
    }
    for (QString key : optimizedTableFields.keys()) {
        QString val = optimizedTableFields.value(key).toString();
        if (key.endsWith("*")) {
            key = key.remove("*");
        }
        optimizedTableWildcards << key;
        tableFields << (key + " " + val);
    }
    if (!db.tables().contains(table) || !db.tables().contains(table + "_keys")) {
        if (driver == "QPSQL") {
            if (partitioning) {
                db.exec("CREATE TABLE IF NOT EXISTS " + table + "(id BIGSERIAL NOT NULL, tstmp TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL, " +
                        tableFields.join(", ") + ", flow JSONB NOT NULL) PARTITION BY RANGE(tstmp)");
            } else {
                db.exec("CREATE TABLE IF NOT EXISTS " + table + "(id BIGSERIAL NOT NULL, tstmp TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL, " +
                        tableFields.join(", ") + ", flow JSONB NOT NULL)");
            }
            if (db.lastError().type() != QSqlError::NoError) {
                qInfo() << db.lastError().databaseText();
            }
            if (compressKeys) {
                db.exec("CREATE TABLE IF NOT EXISTS " + table + "_keys(name VARCHAR NOT NULL UNIQUE, value VARCHAR NOT NULL UNIQUE)");
                if (db.lastError().type() != QSqlError::NoError) {
                    qInfo() << db.lastError().databaseText();
                }
            }
        } else {
            db.exec("CREATE TABLE " + table + "(id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, tstmp TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL," +
                    tableFields.join(", ") + ",flow VARCHAR NOT NULL)");
            if (compressKeys) {
                db.exec("CREATE TABLE " + table + "_keys(name VARCHAR NOT NULL, value VARCHAR NOT NULL)");
            }
            if (db.lastError().type() != QSqlError::NoError) {
                qInfo() << db.lastError().databaseText();
            }
        }
    }
    if (compressKeys) {
        qInfo() << "Loading compress table";
        compressTable.clear();
        QSqlQuery q = db.exec("SELECT name,value FROM " + table + "_keys");
        while (q.next()) {
            compressTable.insert(q.value(0).toString(), q.value(1).toString());
            qInfo() << q.value(0).toString() << q.value(1).toString();
        }
        qInfo() << "Compress table loaded";
    }
    // begin transaction
    db.transaction();
}
