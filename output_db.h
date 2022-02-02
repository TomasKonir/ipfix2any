#ifndef OUTPUT_DB_H
#define OUTPUT_DB_H

#include <QSqlDatabase>
#include <QJsonObject>
#include "output.h"

class OutputDb : public Output {
public:
	OutputDb(int queueLimit, const QJsonObject &params);
	void next(const output_row_t &row) override;

private:
	void openDb();

private:
	QJsonObject  params;
	QString      table;
	QString      driver;
	QSqlDatabase db;
};

#endif
