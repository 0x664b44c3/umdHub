#ifndef UMDMAPPERMANAGER_H
#define UMDMAPPERMANAGER_H

#include <QObject>
#include <QMap>
class QJsonObject;
class QJsonArray;
class TslTransmitter;
class UmdMapper;

class UmdBusManager : public QObject
{
	Q_OBJECT
public:
	explicit UmdBusManager(QObject *parent = nullptr);
	bool createBusFromJson(const QJsonObject &);
	QJsonArray getJson() const;

private:
	int mBusNumber;

	TslTransmitter * createTslBus(QString params);

	struct BusObject
	{
		QString description;
		QString driverCfg;
		TslTransmitter * channelDriver;
		UmdMapper * mapper;
	};
	QMap<QString, BusObject> mBusses;
signals:

public slots:
};

#endif // UMDMAPPERMANAGER_H
