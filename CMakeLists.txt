cmake_minimum_required(VERSION 3.14)

project(ipfix2any LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Core Network Sql)
find_package(Qt6 REQUIRED COMPONENTS Core Network Sql)

add_executable(ipfix2any
    main.cpp convert.cpp filter_fields.cpp filter_skip.cpp filter_zero.cpp ipfix.cpp output.cpp output_null.cpp output_stdout.cpp output_db.cpp
    convert.h filter_fields.h filter_skip.h filter_zero.h ipfix.h output.h output_null.h output_stdout.h output_db.h
    ipfix.qrc
)
target_link_libraries(ipfix2any Qt6::Core)
target_link_libraries(ipfix2any Qt6::Network)
target_link_libraries(ipfix2any Qt6::Sql)

include(GNUInstallDirs)
install(TARGETS ipfix2any
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

add_custom_target(deb DEPENDS all)
add_custom_command(TARGET deb POST_BUILD COMMAND ${CMAKE_SOURCE_DIR}/deb-build/mkdeb.sh WORKING_DIRECTORY ${CMAKE_BUILD_DIR})
