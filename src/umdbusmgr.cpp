#include "umdbusmgr.h"
#include <QJsonObject>
#include <tsltransmitter.h>
#include <umdmapper.h>
#include <QSerialPort>
#include <umddatabase.h>
#include <QDebug>
UmdBusManager::UmdBusManager(QObject *parent) : QObject(parent),
    mBusNumber(0)
{

}

bool UmdBusManager::createBusFromJson(const QJsonObject & obj)
{
	QJsonValue uid = obj.value("uid");
	QJsonValue driver = obj.value("driver");
	QJsonValue map = obj.value("mappings");

	if (
	        (driver.isUndefined()) ||
	        (!map.isArray())
	        )
	{
		qWarning()<<"Illegal config object.";
		return false;
	}

	QString driverString = obj.value("driver").toString();
	int idx = driverString.indexOf(':');
	QString driverName = driverString.left(idx);
	QString driverParams = driverString.mid(idx+1);
	QString uidString = uid.toString();
	if (uid.isUndefined())
	{
		uidString = "umdbus_" + driverName + QString("_%1").arg(mBusNumber++);
	}
	if (driverName == "tsl")
	{
		TslTransmitter * channel = createTslBus(driverParams);
		if (!channel)
		{
			qDebug()<<"Could not create tsl bus for"<<driverParams;
		}
		UmdMapper *mapper = new UmdMapper(this);
		mapper->setChannel(channel);
		bool ok = mapper->initFromJson(map.toArray());
		if (!ok)
		{
			if (channel)
				channel->deleteLater();
			mapper->deleteLater();
			return false;
		}
		BusObject busObj;
		busObj.driverCfg = obj.value("driver").toString();
		busObj.channelDriver = channel;
		busObj.mapper = mapper;
		busObj.description = obj.value("desc").toString();
		mBusses.insert(uidString, busObj);
		connect(UmdDB::inst(), SIGNAL(umdChanged(QString,int,UmdDB::umdInfo)),
		        mapper, SLOT(onUmdChanged(QString,int,UmdDB::umdInfo)));
		qDebug()<<"created bus"<<uidString<<busObj.driverCfg;

	}
	return true;
}

QJsonArray UmdBusManager::getJson() const
{
	QJsonArray array;
	for(auto it=mBusses.begin(); it!=mBusses.end(); ++it)
	{
		QJsonObject busObj;
		BusObject bus = it.value();
		busObj.insert("uid", it.key());
		busObj.insert("driver", bus.driverCfg);
		busObj.insert("desc", bus.description);
		busObj.insert("mappings", bus.mapper->getJson());
		array.append(busObj);
	}
	return array;
}

TslTransmitter *UmdBusManager::createTslBus(QString parameters)
{
	QStringList params = parameters.split(':');
	if (params.isEmpty())
		return nullptr;
	QString channelType = params.takeFirst();
	if (params.isEmpty())
		return nullptr;
	QString channelId = params.takeFirst();


	TslTransmitter *tsl = new TslTransmitter(this);
	if (channelType == "uart")
	{
		QSerialPort * sp = new QSerialPort(tsl);
		sp->setPortName(channelId);
		sp->setBaudRate(38400);
		sp->setParity(QSerialPort::EvenParity);
		if (!sp->open(QIODevice::ReadWrite))
		{
			tsl->deleteLater();
			return nullptr;
		}
		tsl->setStreamDevice(sp, qMax(32, sp->baudRate() / 150));
		if (params.contains("legacy"))
			tsl->setTslMode(true);
		connect(sp, SIGNAL(error(QSerialPort::SerialPortError)),
		        tsl, SLOT(onChannelError()));
		return tsl;
	}
	if (channelType == "udp")
	{
		return nullptr;
	}
	tsl->deleteLater();
	return nullptr;
}
