#ifndef CONFIGFILE_H
#define CONFIGFILE_H

#include <QObject>
#include <QByteArray>
class ConfigFile : public QObject
{
	Q_OBJECT
public:
	explicit ConfigFile(QObject *parent = nullptr);
	~ConfigFile();
	static QByteArray dumpConfig();
	bool loadAutosave(bool loadUmdDb = true, bool loadBusses=false);

private:
	bool mAutoSave;
	QString mFileName;
signals:

public slots:

};

#endif // CONFIGFILE_H
