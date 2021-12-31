# ipfix2any

Simple ipfix receiver.

Receive as json. other targets will be added later.

Tested on mikrotik IPFIX and INVEA flowmon ipfix.

Missing fields can be simply added to ipfix.json.

Compile like other qt projects with qmake and make.

mkdir build
cd build
qmake ../ipfix2any.pro
make

