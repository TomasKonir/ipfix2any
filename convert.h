#ifndef CONVERT_H
#define CONVERT_H

#include <QString>

QString convertAny(const char *data, long length);
quint64 getSignedNum(const char *data, long length);
quint64 getUnsignedNum(const char *data, long length);
QString msecs2string(quint64 n);
QString convertSignedNumber(const char *data, long length);
QString convertUnsignedNumber(const char *data, long length);
QString convertProtocol(const char *data, long length);
QString convertIpv4(const char *data, long length);
QString convertIpv6(const char *data, long length);
QString convertMac(const char *data, long length);
QString convertUptimeMs(const char *data, long length);
QString convertDateTimeMs(const char *data, long length);
QString convertTcpControlBits(const char *data, long length);
QString convertString(const char *data, long length);

#endif // CONVERT_H
