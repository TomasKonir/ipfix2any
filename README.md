# ipfix2any

Simple ipfix receiver.

Receive IPFIX from any source and outputs it as JSON, NULL, (postgres, sqlite and maybe others will come).

Tested on mikrotik IPFIX and INVEA flowmonexp ipfix (INVEA is now FLOWMON .... wait .... FLOWMON is now Progress :-).

Missing fields can be simply added to ipfix.json.

Compile like other qt projects with qmake and make.

    mkdir build
    cd build
    qmake ../ipfix2any.pro
    make

Usage (default config is builtin): 

    ipfix2any "[configFile|showDefault]"
