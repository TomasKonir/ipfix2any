#ifndef OUTPUT_DB_H
#define OUTPUT_DB_H

#include <QSqlDatabase>
#include <QJsonObject>
#include <QDateTime>
#include "output.h"

class OutputDb : public Output {
public:
	OutputDb(int queueLimit, QList<Filter *> filterList, const QJsonObject &params);
	~OutputDb();
	void next(const output_row_t &row) override;
	void keyAdded(QHash<QString,QString> *table, QString name, QString value);

private:
	void openDb();

private:
	QJsonObject  params;
	QString      table;
	QString      driver;
	QSqlDatabase db;
	bool         compressKeys;
	QDateTime    lastCommit;
    bool         partitioning;
	QHash<QString,QString> compressTable;
};

#endif
