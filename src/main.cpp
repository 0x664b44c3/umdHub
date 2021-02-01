#include <QCoreApplication>
#include <tsltransmitter.h>
#include <QSerialPort>
#include <QDebug>
#include <simpleprotocol.h>
#include <umddatabase.h>
#include <protocollistener.h>
#include <configfile.h>
#include "unix_signal.h"
#include <globals.h>
#include "umdbusmgr.h"

namespace Global {
    UmdBusManager *BusMgr = nullptr;
	ConfigFile *CfgFile= nullptr;
}

auto quitSignalHandler = [](int sig) -> void {
	// blocking and not aysnc-signal-safe func are valid
	printf("\nquit the application by signal(%d).\n", sig);
	QCoreApplication::quit();
};

int main(int argc, char *argv[])
{

	QCoreApplication a(argc, argv);
	catchUnixSignals({SIGQUIT, SIGINT, SIGTERM, SIGHUP}, quitSignalHandler);
	//create umd db singleton
	UmdDB::inst();
	ProtocolListener pl(&a);

	UmdBusManager busMgr(&a);
	ConfigFile cfgFile(&a);
	Global::BusMgr = &busMgr;
	Global::CfgFile= &cfgFile;

	cfgFile.loadAutosave(true, true);

	return a.exec();
}
