#include <QDebug>
#include <QtEndian>
#include <QJsonObject>
#include <QDateTime>
#include "ipfix.h"
#include "convert.h"
#include "output_null.h"
#include "output_stdout.h"
#include "output_db.h"

IPFIX::IPFIX(const QJsonArray fd, long ql, const QJsonObject fixes, const QJsonArray outputs, bool d) : debug(d), queueLimit(ql){
	mikrotikFixTimestamp = fixes.value("mikrotikFixTimestamp").toBool(false);
	for(const auto &v : fd){
		if(!v.isObject()){
			qInfo() << "Invalid field:" << v;
		} else {
			QJsonObject o = v.toObject();
			bool failed = false;
			for(const auto &s : QStringList({"id","name","type"})){
				if(!o.contains(s) || !o.value(s).isString()){
					qInfo() << "Missing or invalid field:" << s;
					failed = true;
					break;
				}
			}
			if(!failed){
				field_def_t fdt;
				QString type = o.value("type").toString();
				fdt.id = o.value("id").toString();
				fdt.name = o.value("name").toString();
				if(type == "signed"){
					fdt.convertFunc = convertSignedNumber;
				} else if(type == "unsigned"){
					fdt.convertFunc = convertUnsignedNumber;
				} else if(type == "protocol"){
					fdt.convertFunc = convertProtocol;
				} else if(type == "ipv4addr"){
					fdt.convertFunc = convertIpv4;
				} else if(type == "ipv6addr"){
					fdt.convertFunc = convertIpv6;
				} else if(type == "mac_addr"){
					fdt.convertFunc = convertMac;
				} else if(type == "uptime_ms"){
					fdt.convertFunc = convertUptimeMs;
				} else if(type == "date_time_ms"){
					fdt.convertFunc = convertDateTimeMs;
				} else if(type == "tcp_bits"){
					fdt.convertFunc = convertTcpControlBits;
				} else if(type == "icmp_type"){
					fdt.convertFunc = convertUnsignedNumber;
				} else if(type == "icmp_code"){
					fdt.convertFunc = convertUnsignedNumber;
				} else if(type == "icmp_type_code"){
					fdt.convertFunc = convertUnsignedNumber;
				} else if(type == "igmp_type"){
					fdt.convertFunc = convertUnsignedNumber;
				} else if(type == "string"){
					fdt.convertFunc = convertString;
				} else if(type == "natEvent"){
					fdt.convertFunc = convertNatEvent;
				} else {
					qInfo() << "Unknown type" << o;
					fdt.convertFunc = convertAny;
				}
				fieldDefs.insert(fdt.id,fdt);
			}
		}
	}
	if(outputs.count() == 0){
		qInfo() << "No output given ...";
	} else {
		for(const auto &v : outputs){
			if(!v.isObject() || !v.toObject().value("name").isString()){
				qInfo() << "Invalid output:" << v;
				continue;
			}
			QJsonObject o = v.toObject();
			QJsonObject params = o.value("params").toObject();
			QString name = o.value("name").toString();
			if(name == "null"){
				outputList << new OutputNull(queueLimit);
			} else if(name == "stdout"){
				outputList << new OutputStdout(queueLimit);
			} else if(name == "db"){
				outputList << new OutputDb(queueLimit,params);
			} else {
				qInfo() << "Unknown output:" << name;
			}
		}
	}
	start();
}

IPFIX::~IPFIX(){
	for(const auto &o : outputList){
		if(o->isRunning()){
			qInfo() << "Waiting for output finish";
			o->requestInterruption();
			o->wait(5000);
		}
		if(!o->isRunning()){
			delete(o);
		}
	}
	outputList.clear();
}

void IPFIX::run(){
	while(true){
		mutex.lock();
		if(!queue.isEmpty()){
			work_block_t wb = queue.dequeue();
			mutex.unlock();
			next(wb.data,wb.addr);
		} else {
			waitCondition.wait(&mutex,1000);
			mutex.unlock();
		}
		if(isInterruptionRequested()){
			break;
		}
	}
}

void IPFIX::enqueue(const QByteArray &data, const QHostAddress &addr){
	mutex.lock();
	if(queue.count() > queueLimit){
		qInfo() << "Queue limit reached";
	}
	queue.enqueue(work_block_t{data,addr});
	waitCondition.wakeOne();
	mutex.unlock();
}


void IPFIX::next(const QByteArray &data, const QHostAddress &addr){
	unsigned long length = data.size();
	unsigned long offset = 0;
	const char *block = data.constData();
	ipfix_hdr_t hdr;

	if(length < sizeof(hdr)){
		return;
	}

	offset += hdrGet(hdr,block);
	if(hdr.length != length){
		qInfo() << "Invalid length" << length << hdr.length;
		return;
	}
	if(hdr.version != IPFIX_VERSION){
		return;
	}
	QString ident = addr.toString() + "_" + QString::number(hdr.observationDomainID);
	while(offset < length){
		unsigned long ret = nextSet(block + offset, length - offset,ident,hdr.exportTime);
		offset += ret;
		if(ret == 0){
			qInfo() << "Invalid block";
			break;
		}
	}
	if(offset != length){
		qInfo() << "Invalid block" << offset << length;
	}
}

unsigned long IPFIX::hdrGet(ipfix_hdr_t &hdr,const char *data){
	const ipfix_hdr_t *sHdr = reinterpret_cast<const ipfix_hdr_t*>(data);
	hdr.exportTime = qFromBigEndian(sHdr->exportTime);
	hdr.length = qFromBigEndian(sHdr->length);
	hdr.observationDomainID = qFromBigEndian(sHdr->observationDomainID);
	hdr.sequenceNumber = qFromBigEndian(sHdr->sequenceNumber);
	hdr.version = qFromBigEndian(sHdr->version);
	if(debug) qInfo() << "IPFIX:" << hdr.exportTime << hdr.length << hdr.observationDomainID << hdr.sequenceNumber << hdr.version;
	return(sizeof(ipfix_hdr_t));
}

unsigned long IPFIX::nextSet(const char *data, long remaing, QString ident, quint32 exportTime){
	const ipfix_set_hdr_t *sHdr = reinterpret_cast<const ipfix_set_hdr_t*>(data);
	ipfix_set_hdr_t hdr;

	hdr.setID = qFromBigEndian(sHdr->setID);
	hdr.fieldLength = qFromBigEndian(sHdr->fieldLength);

	if(hdr.fieldLength > remaing){
		qInfo() << "Invalid field length";
		return(0);
	}

	if(hdr.setID == 2){
		if(debug) qInfo() << "SET  : TEMPLATE" << hdr.setID << hdr.fieldLength;
		processTemplates(data + sizeof(hdr),hdr.fieldLength - sizeof(hdr),ident);
	} else if(hdr.setID >= 256){
		if(debug) qInfo() << "SET  : DATA    " << hdr.setID << hdr.fieldLength << remaing;
		processDataset(data + sizeof(hdr),hdr.fieldLength - sizeof(hdr),hdr.setID,ident,exportTime);
	} else {
		if(debug) qInfo() << "SET  : OTHER   " << hdr.setID << hdr.fieldLength;
	}
	return(hdr.fieldLength);
}


void IPFIX::processTemplates(const char *data, long remain, QString ident){
	long hdrSize = (sizeof(ipfix_template_hdr_t)+4);
	while(remain > hdrSize){
		ipfix_template_t t;
		const ipfix_template_hdr_t *sHdr = reinterpret_cast<const ipfix_template_hdr_t*>(data);
		ipfix_template_hdr_t hdr;
		hdr.templateID = qFromBigEndian(sHdr->templateID);
		hdr.fieldCount = qFromBigEndian(sHdr->fieldCount);
		if(hdr.templateID < 256){
			qInfo() << "Invalid template" << hdr.templateID << hdr.fieldCount;
			return;
		}
		if(debug) qInfo() << "TEMPLATE:" << hdr.templateID << hdr.fieldCount;
		data += sizeof(hdr);
		remain -= sizeof(hdr);
		t.baseSize = 0;
		for(int i=0;i<hdr.fieldCount;i++){
			const ipfix_element_hdr_t *sElement = reinterpret_cast<const ipfix_element_hdr_t*>(data);
			ipfix_element_hdr_t element;
			bool enterprise;
			element.informationElementId = qFromBigEndian(sElement->informationElementId);
			element.fieldLength = qFromBigEndian(sElement->fieldLength);
			enterprise = element.informationElementId & 0x8000;
			element.informationElementId &= 0x7FFF;
			if(debug) qInfo() << "ELEMENT:" << enterprise << element.informationElementId << element.fieldLength;

			ipfix_field_t f;
			f.enterprise = enterprise;
			f.informationElementId = element.informationElementId;
			f.fieldLength = element.fieldLength;
			f.enterpriseNumber = enterprise ? qFromBigEndian(*(reinterpret_cast<const quint32*>(data + sizeof(element)))) : 0;

			QString lookup = QString::number(f.informationElementId);
			if(f.enterprise){
				lookup += "_" + QString::number(f.enterpriseNumber);
			}

			if(fieldDefs.contains(lookup)){
				field_def_t fdt = fieldDefs.value(lookup);
				f.name = fdt.name;
				f.convertFunc = fdt.convertFunc;
			} else {
				f.name = "unknown_" + QString::number(f.informationElementId);
				f.convertFunc = convertAny;
				if(f.enterprise){
					f.name += "_" + QString::number(f.enterpriseNumber);
				}
			}

			t.fields << f;
			if(f.fieldLength == 0xFFFF){
				t.baseSize += 1;
			} else {
				t.baseSize += f.fieldLength;
			}

			data += sizeof(element) + (enterprise ? 4 : 0);
			remain -= sizeof(element) + (enterprise ? 4 : 0);
			if(remain < 0){
				break;
			}
		}
		if(remain < 0){
			qInfo() << "Invalid data" << remain;
		} else {
			templates.insert(ident + "_"  + QString::number(hdr.templateID),t);
		}
	}
	if(remain > 0){
		if(debug) qInfo() << "PADDING" << remain;
	}
}

void IPFIX::processDataset(const char *data, long remaing, int id, QString ident, quint32 exportTime){
	quint64 now = QDateTime::currentMSecsSinceEpoch();
	QString index = ident + "_" + QString::number(id);
	QString exportTimeString = QDateTime::fromSecsSinceEpoch(exportTime).toString("dd.MM.yyyyThh:mm:ss");
	QString nowTimeString = QDateTime::currentDateTime().toString("dd.MM.yyyyThh:mm:ss.zzz");
	if(!templates.contains(index)){
		qInfo() << "Missing template" << ident << id;
		return;
	}
	ipfix_template_t t = templates.value(index);
	while(remaing >= t.baseSize){
		output_row_t row;
		quint64 realFlowStart = 0;
		quint64 realFlowEnd = 0;
		quint64 systemStartup = 0;
		row << output_field_t{"ident","\"" + ident + "\"",0,0,QByteArray()};
		row << output_field_t{"exportTime","\"" + exportTimeString + "\"",0,0,exportTimeString.toUtf8()};
		row << output_field_t{"collectedTime","\"" + nowTimeString + "\"",0,0,nowTimeString.toUtf8()};
		for(int i=0;i<t.fields.count();i++){
			ipfix_field_t f = t.fields.at(i);
			int fl = f.fieldLength;
			if(fl == 65535){
				fl = reinterpret_cast<const quint8*>(data)[0];
				data++;
				remaing--;
				if(fl == 255){
					fl = qFromBigEndian(reinterpret_cast<const quint16*>(data)[0]);
					data += 2;
					remaing -= 2;
				}
			}
			if(remaing < fl){
				qInfo() << "Invalid record: " << remaing << fl << i << t.fields.count();
				return;
			}
			if(debug) qInfo() << "Field:" << f.name << fl << remaing;
			if(mikrotikFixTimestamp && !f.enterprise){
				if(f.informationElementId == IPFIX_ELEMENT_STARTUP_TIME){
					systemStartup = getUnsignedNum(data,fl);
				}
				if(f.informationElementId == IPFIX_ELEMENT_FLOW_START_UPTIME){
					realFlowStart = getUnsignedNum(data,fl);
				}
				if(f.informationElementId == IPFIX_ELEMENT_FLOW_END_UPTIME){
					realFlowEnd = getUnsignedNum(data,fl);
				}
			}
			row << output_field_t{f.name,f.convertFunc(data,fl),f.informationElementId,f.enterpriseNumber,QByteArray(data,fl)};
			data += fl;
			remaing -= fl;
		}
		if(mikrotikFixTimestamp && systemStartup > 0 && realFlowStart > 0 && realFlowEnd > 0){
			while(now > systemStartup && (now - systemStartup) > 0x00000000FFFFFFFFULL){
				systemStartup += 0x00000000FFFFFFFFULL;
			}
			realFlowStart += systemStartup;
			realFlowEnd   += systemStartup;
			quint64 stmp = qToBigEndian(realFlowStart);
			quint64 etmp = qToBigEndian(realFlowEnd);
			row << output_field_t{"flowStartMilliseconds",msecs2string(realFlowStart),152,0,QByteArray(reinterpret_cast<const char*>(&stmp),8)};
			row << output_field_t{"flowEndMilliseconds",msecs2string(realFlowEnd),153,0,QByteArray(reinterpret_cast<const char*>(&etmp),8)};
		}
		for(const auto &o : outputList){
			o->enqueue(row);
		}
	}
	if(remaing > 0){
		if(debug) qInfo() << "Field padding:" << remaing;
	}
}
