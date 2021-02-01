#ifndef UMDMAPPER_H
#define UMDMAPPER_H

#include <QObject>
#include <QMap>
#include <tsltransmitter.h>
#include <umddatabase.h>
#include <QJsonArray>
struct umdMapping
{
	int tslChannel;
	QString umdTemplate;
	QList<int> tallyMap;
};
class UmdMapper : public QObject
{
	Q_OBJECT
public:
	explicit UmdMapper(QObject *parent = nullptr);
	bool channelMapped(int channel) const;
	void deleteMappings(const QString & uid);
	void deleteMapping(int channel);
	void setChannel(TslTransmitter * xmitter);
	bool setMapping(QString uid,
	                int channel,
	                QString umdTemplate = QString(),
	                QVector<int> tallyMap = QVector<int>());
	bool initFromJson(const QJsonArray & mappings);
	QJsonArray getJson() const;
private slots:
	void onUmdChanged(QString uid, int changedMask, UmdDB::umdInfo properties);

private:
	QMultiMap<QString, umdMapping> mMappings;
	//impl dependant
	TslTransmitter * mTsl;
signals:

public slots:
};

#endif // UMDMAPPER_H
