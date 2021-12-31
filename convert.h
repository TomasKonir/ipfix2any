#ifndef CONVERT_H
#define CONVERT_H

#include <QDebug>
#include <QString>
#include <QtEndian>
#include <QHostAddress>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <netdb.h>

QString convertAny(const char *data, long length){
	return("\"" + QString(QByteArray(data,length).toHex()) + "\"");
}

quint64 getSignedNum(const char *data, long length){
	qint64 num = 0;
	switch(length){
		case 1:
			num = reinterpret_cast<const qint8*>(data)[0];
			break;
		case 2:
			num = qFromBigEndian(reinterpret_cast<const qint16*>(data)[0]);
			break;
		case 4:
			num = qFromBigEndian(reinterpret_cast<const qint32*>(data)[0]);
			break;
		case 8:
			num = qFromBigEndian(reinterpret_cast<const qint64*>(data)[0]);
			break;
		default:
			for(int i=0;i<length;i++){
				num <<= 8;
				num |= reinterpret_cast<const qint8*>(data)[0];
			}
			break;
	}
	return(num);
}

quint64 getUnsignedNum(const char *data, long length){
	quint64 num = 0;
	switch(length){
		case 1:
			num = reinterpret_cast<const quint8*>(data)[0];
			break;
		case 2:
			num = qFromBigEndian(reinterpret_cast<const quint16*>(data)[0]);
			break;
		case 4:
			num = qFromBigEndian(reinterpret_cast<const quint32*>(data)[0]);
			break;
		case 8:
			num = qFromBigEndian(reinterpret_cast<const quint64*>(data)[0]);
			break;
		default:
			for(int i=0;i<length;i++){
				num <<= 8;
				num |= reinterpret_cast<const quint8*>(data)[0];
			}
			break;
	}
	return(num);
}

QString convertSignedNumber(const char *data, long length){
	return(QString::number(getSignedNum(data,length)));
}

QString convertUnsignedNumber(const char *data, long length){
	return(QString::number(getUnsignedNum(data,length)));
}

QString convertProtocol(const char *data, long length){
	if(length > 1){
		return(convertAny(data,length));
	}quint8 pn = reinterpret_cast<const qint8*>(data)[0];
	struct protoent *p = getprotobynumber(pn);
	if(p){
		return("\"" + QString::fromLatin1(p->p_name) + "\"");
	} else {
		return("\"unknown (" + QString::number(pn) + ")\"");
	}
}

QString convertIpv4(const char *data, long length){
	if(length != 4){
		return(convertAny(data,length));
	}
	QHostAddress ha(qFromBigEndian(reinterpret_cast<const quint32*>(data)[0]));
	return("\"" + ha.toString() + "\"");
}

QString convertIpv6(const char *data, long length){
	if(length != 16){
		return(convertAny(data,length));
	}
	QHostAddress ha(reinterpret_cast<const quint8*>(data));
	return("\"" + ha.toString() + "\"");
}

QString convertMac(const char *data, long length){
	QStringList list;
	for(int i=0;i<length;i++){
		list << QByteArray(data+i,1).toHex();
	}
	return("\"" + list.join(':') + "\"");
}

QString convertUptimeMs(const char *data, long length){
	return(convertUnsignedNumber(data,length));
}

QString convertDateTimeMs(const char *data, long length){
	QDateTime t = QDateTime::fromMSecsSinceEpoch(getUnsignedNum(data,length));
	return("\"" + t.toString("dd.MM.yyyyThh:mm:ss.zzz") + "\"");
}

QString convertTcpControlBits(const char *data, long length){
	quint64 bits = getUnsignedNum(data,length);
	QStringList tcp;
	if(bits & 0x0100) tcp <<  "\"NS\"";
	if(bits & 0x0080) tcp <<  "\"CWR\"";
	if(bits & 0x0040) tcp <<  "\"ECE\"";
	if(bits & 0x0020) tcp <<  "\"URG\"";
	if(bits & 0x0010) tcp <<  "\"ACK\"";
	if(bits & 0x0008) tcp <<  "\"PSH\"";
	if(bits & 0x0004) tcp <<  "\"RST\"";
	if(bits & 0x0002) tcp <<  "\"SYN\"";
	if(bits & 0x0001) tcp <<  "\"FIN\"";
	return("[" + tcp.join(',') + "]");
}

QString convertString(const char *data, long length){
	const char zero = 0;
	QByteArray ba(data,length);
	if(ba.contains(zero)){
		ba.truncate(ba.indexOf(zero));
	}
	QString s = QString::fromUtf8(ba);
	QJsonArray a;
	a << s;
	QJsonDocument d(a);
	s = QString::fromUtf8(d.toJson(QJsonDocument::Compact));
	s.remove(0,1);
	s.remove(s.length() - 1,1);
	qInfo().noquote() << s;
	return(s);
}

#endif // CONVERT_H
