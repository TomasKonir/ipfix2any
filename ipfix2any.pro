QT -= gui
QT += network

TARGET = ipfix2any

CONFIG += c++11 console
CONFIG -= app_bundle

SOURCES += main.cpp \
	convert.cpp \
	ipfix.cpp \
	output.cpp \
	output_null.cpp \
	output_stdout.cpp

HEADERS += \
	convert.h \
	ipfix.h \
	output.h \
	output_null.h \
	output_stdout.h

RESOURCES += ipfix.qrc

#Compile with asan sometimes
LIBS           += -lasan
QMAKE_LFLAGS   += -lasan -fsanitize=address

