#include "configfile.h"
#include <umddatabase.h>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QDebug>
#include <QStandardPaths>
#include <QFile>
#include <QDir>
#include "globals.h"
#include <umdbusmgr.h>

ConfigFile::ConfigFile(QObject *parent) : QObject(parent), mAutoSave(false)
{
	mAutoSave = true;
}

ConfigFile::~ConfigFile()
{
	QString filePath = QStandardPaths::writableLocation( QStandardPaths::AppConfigLocation	);

	QByteArray cfg = dumpConfig();
	QDir dir;
	dir.mkpath(filePath);
	filePath+="/umd_hub.autosave";
	QFile saveFile(filePath);
	if (saveFile.open(QFile::WriteOnly|QFile::Truncate))
	{
		saveFile.write(cfg);
		saveFile.close();
		qDebug()<<"State saved to:" << filePath;
	}
	else
	{
		qDebug()<<"Cannot open/create save file:" << filePath;
		qDebug()<<saveFile.errorString();
		qDebug()<<cfg;
	}
}

QByteArray ConfigFile::dumpConfig()
{
	QJsonObject config;
	config.insert("umd_db", UmdDB::inst()->getJson());
	config.insert("umd_busses", Global::BusMgr->getJson());
	QJsonDocument jdoc;
	jdoc.setObject(config);
	return jdoc.toJson();
}

bool ConfigFile::loadAutosave(bool loadUmdDb, bool loadBusses)
{
	QString filePath = QStandardPaths::writableLocation( QStandardPaths::AppConfigLocation	);
	filePath+="/umd_hub.autosave";
	QFile saveFile(filePath);
	if (saveFile.open(QFile::ReadOnly))
	{
		QByteArray data = saveFile.readAll();
		QJsonParseError error;
		QJsonDocument jdoc = QJsonDocument::fromJson(data, &error);
		if ((error.error == QJsonParseError::NoError) && jdoc.isObject())
		{
			qDebug()<<"Reading save file";
			QJsonObject object = jdoc.object();
			QJsonValue umd_db = object.value("umd_db");
			QJsonValue busses = object.value("umd_busses");
			if ((loadBusses) && busses.isArray())
			{
				foreach(QJsonValue bjv, busses.toArray())
				{
					if (bjv.isObject())
					{
						Global::BusMgr->createBusFromJson(bjv.toObject());
					}
				}
			}
			if ((loadUmdDb) && (umd_db.isObject()))
			{
				UmdDB::inst()->loadJson(umd_db.toObject());
			}
		}
		else
		{
			qWarning()<<__FILE__<<":"<<__LINE__<<error.errorString()
			         << "at offset" << error.offset;
		}
	}
	return false;
}
