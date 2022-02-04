#ifndef OUTPUT_H
#define OUTPUT_H

#include <QString>
#include <QQueue>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>

typedef struct {
	QString    name;
	QString    value;
	quint16    id;
	quint32    enterpriseId;
	QByteArray raw;
}output_field_t;

typedef struct {
	QString					templateId;
	QList<output_field_t>	fields;
} output_row_t;

class Output : public QThread {
public:
	Output(int queueLimit);
	void enqueue(const output_row_t &row);
	void run();

	//helper
	QString row2json(const output_row_t &row, QHash<QString, QString> *compressTable = nullptr, bool autoAdd = false, QHash<QString,QString> *addedKeys = nullptr);

	//need to be implemented in subclasses
	virtual void next(const output_row_t &row) = 0;

protected:
	int								queueLimit;
	QQueue<output_row_t>			queue;
	QMutex							mutex;
	QWaitCondition					waitCondition;
};

#endif
