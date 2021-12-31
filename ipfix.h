#ifndef IPFIX_H
#define IPFIX_H

#include <QByteArray>
#include <QList>
#include <QHash>
#include <QJsonArray>
#include <QTextStream>
#include <QHostAddress>
#include <QThread>
#include <QQueue>
#include <QMutex>
#include <QWaitCondition>
#include "output.h"

#define IPFIX_VERSION 10

#define IPFIX_ELEMENT_STARTUP_TIME      160
#define IPFIX_ELEMENT_FLOW_START_UPTIME  22
#define IPFIX_ELEMENT_FLOW_END_UPTIME    21

typedef QString (*fieldToString)(const char *data, long len);

typedef struct {
	quint16 version;
	quint16 length;
	quint32 exportTime;
	quint32 sequenceNumber;
	quint32 observationDomainID;
}__attribute__((packed))ipfix_hdr_t;

typedef struct {
	quint16 setID;
	quint16 fieldLength;
}__attribute__((packed))ipfix_set_hdr_t;

typedef struct {
	quint16 templateID;
	quint16 fieldCount;
}__attribute__((packed))ipfix_template_hdr_t;

typedef struct {
	quint16 informationElementId;
	quint16 fieldLength;
}__attribute__((packed))ipfix_element_hdr_t;

typedef struct {
	bool enterprise;
	quint16  informationElementId;
	quint16  fieldLength;
	quint32  enterpriseNumber;
	QString  name;
	fieldToString convertFunc;
}ipfix_field_t;

typedef struct {
	long                 baseSize;
	QList<ipfix_field_t> fields;
}ipfix_template_t;

typedef struct {
	QString		  id;
	QString       name;
	fieldToString convertFunc;
}field_def_t;

typedef struct {
	QByteArray data;
	QHostAddress addr;
}work_block_t;

class IPFIX: public QThread
{
public:
	IPFIX(const QJsonArray fieldDefs, long queueLimit, const QJsonObject fixes, const QJsonArray outputs, bool debug);
	~IPFIX();
	void enqueue(const QByteArray &data, const QHostAddress &addr);
	void next(const QByteArray &data, const QHostAddress &addr);
	void run();

private:
	unsigned long hdrGet(ipfix_hdr_t &hdr,const char *data);
	unsigned long nextSet(const char *data, long remaing, QString ident, quint32 exportTime);
	void          processTemplates(const char *data, long remain, QString ident);
	void          processDataset(const char *data, long remaing, int id, QString ident, quint32 exportTime);

private:
	QHash<QString,field_def_t>      fieldDefs;
	QHash<QString,ipfix_template_t> templates;
	bool							mikrotikFixTimestamp;
	bool                            debug;
	QQueue<work_block_t>			queue;
	long                            queueLimit;
	QMutex						    mutex;
	QWaitCondition                  waitCondition;
	QList<Output*>					outputList;
};

#endif // IPFIX_H

