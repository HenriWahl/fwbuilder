include(../tests_common.pri)

TARGET = PIXImporterTest
HEADERS += PIXImporterTest.h
SOURCES += main_PIXImporterTest.cpp \
        PIXImporterTest.cpp
 
STATIC_LIBS +=  ../../libgui/libgui.a
