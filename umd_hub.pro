QT -= gui
QT += network
QT += serialport
CONFIG += c++11 console
CONFIG -= app_bundle

TARGET = umd_hub
# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH+=$$PWD/src
SOURCES += \
		  src/main.cpp \
	 src/simpleprotocol.cpp \
	 src/umddatabase.cpp \
	 src/umdmapper.cpp \
	 src/tsltransmitter.cpp \
	 src/protocollistener.cpp \
	 src/configfile.cpp \
	 src/unix_signal.cpp \
	 src/umdbusmgr.cpp
include($$PWD/../umd_tools/tslDump/tsl.pri)

HEADERS += \
	 src/simpleprotocol.h \
	 src/umddatabase.h \
	 src/umdmapper.h \
	 src/tsltransmitter.h \
	 src/protocollistener.h \
	 src/configfile.h \
	 src/unix_signal.h \
	 src/umdbusmgr.h \
	 src/globals.h
