QT -= gui
QT += network

TARGET = ipfix2any

CONFIG += c++11 console
CONFIG -= app_bundle

SOURCES += main.cpp \
	ipfix.cpp

HEADERS += \
	convert.h \
	ipfix.h

RESOURCES += ipfix.qrc
