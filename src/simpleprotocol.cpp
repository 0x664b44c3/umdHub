#include "simpleprotocol.h"
#include <QDebug>
#include <umddatabase.h>
#include <configfile.h>
enum {
	parserNormal=0,
	parserInBlock,

};

const QRegExp SPACE_REGEX("\\s+");

bool dumpConfig(simpleProtocol* i, QString cmd, QString params)
{
	QByteArray cfgData = ConfigFile::dumpConfig();
	QStringList lines = QString::fromUtf8(cfgData).split("\n");

	i->addResponse(";===BEGIN DUMP===");
	foreach(QString l, lines)
		i->addResponse(";" + l);
	    i->addResponse(";===END DUMP===");
	Q_UNUSED(cmd)
	return true;
}

bool processUID(simpleProtocol* i, QString cmd, QString params)
{
	Q_UNUSED(cmd)
	QStringList plist = params.split(SPACE_REGEX);
	if (!plist.size())
	{
		i->setErrorString("Parameter error");
		return false;
	}
	QString uid = plist.takeFirst();
	UmdDB::umdInfo umd;
	umd.direction=0;
	umd.tally=0;
	int updateMask = 0;
	QMap<QString, QString> blockParameters = i->blockParameters();
	auto it = blockParameters.find("TALLY");
	if (it!=blockParameters.end())
	{
		updateMask|=UmdDB::tallyChanged;
		//		qDebug()<<it.value();
		QStringList tallies = it.value().split(SPACE_REGEX);
		int tally = 0;
		foreach(QString ts, tallies)
		{
			bool ok = false;
			int tallyId = ts.toInt(&ok);
			if (ok)
			{
				if ((tallyId<1) || (tallyId>8))
					continue;
				tally|=(1<<(tallyId-1));
			}
		}
		umd.tally = tally;
	}
	it = blockParameters.find("UMD");
	if (it!=blockParameters.end())
	{
		umd.umd = it.value();
		updateMask|=UmdDB::umdTextChanged;
	}
	it = blockParameters.find("TEXT");
	if (it!=blockParameters.end())
	{
		umd.text = it.value();
		updateMask|=UmdDB::textChanged;
	}
	it = blockParameters.find("MNEMONIC");
	if (it!=blockParameters.end())
	{
		umd.mnemonic = it.value();
		updateMask|=UmdDB::mnemonicChanged;
	}

	UmdDB::inst()->setUmd(uid, umd, updateMask);
	return true;
}


QStringList acceptTokensUID = QStringList()
                              << "MNEMONIC"
                              << "UMD" << "TEXT" << "TALLY" << "DIRECTION";

simpleProtocol::simpleProtocol(QObject *parent) : QObject(parent),mStream(0)
{
	mParserState = parserNormal;
	mBlockCommands.clear();
	mSubscribeAll = false;
	mBlockCommands << "UID";
	registerHandler("UID", processUID);
	registerHandler("dump_json", dumpConfig);
}

simpleProtocol::~simpleProtocol()
{
	//	qDebug()<<"d'tor in"<<__LINE__<<__FILE__;
}

bool simpleProtocol::parseLine(QString line)
{
	QString cmd;
	QString arguments;
	line=line.trimmed();
	int delim = line.indexOf(QRegExp("\\s+"));
	if (delim==0)
		return false;
	if (delim==-1)
	{
		cmd = line;
	}
	else
	{
		cmd = line.left(delim);
		arguments = line.mid(delim+1);
	}

	cmd = cmd.toUpper();

	if (mParserState == parserInBlock)
	{
		if ((cmd==".") || (cmd==""))
		{
			mParserState = parserNormal;
			return processCommand(mCurrentBlockCmd, mCurrentBlockCmdArgs);
		}
		else
		{
			mBlockParameterMap.insert(cmd, arguments);
			return true;
		}
	}
	else
	{
		if ((cmd==".") || (cmd==""))
		{
			return true;
		}
		if (mBlockCommands.contains(cmd))
		{
			mBlockParameterMap.clear();
			mCurrentBlockCmd = cmd;
			mCurrentBlockCmdArgs = arguments;
			mParserState = parserInBlock;
			return true;
		}
		else
		{
			return processCommand(cmd, arguments);
		}
	}
	return false;
}

void simpleProtocol::setStream(QIODevice *dev)
{
	disconnect(this, SLOT(onReadyRead()));
	mStream = dev;
	mRxBuffer.clear();
	connect(mStream, SIGNAL(readyRead()), SLOT(onReadyRead()));
}

QStringList simpleProtocol::lastResonse() const
{
	return mResponseBuffer;
}

QString simpleProtocol::errorString() const
{
	return mErrorString;
}

QIODevice *simpleProtocol::stream() const
{
	return mStream;
}

void simpleProtocol::addResponse(QString s)
{
	mResponseBuffer.push_back(s);
}

QMap<QString, QString> simpleProtocol::blockParameters() const
{
	return mBlockParameterMap;
}


bool simpleProtocol::processCommand(QString cmd, QString args)
{
	qDebug()<<"command handler:"<<cmd<<args;
	if (mHandlers.contains(cmd))
	{
		return mHandlers.value(cmd)(this, cmd, args);
	}
	if (cmd == "LIST")
	{
		foreach(QString uid, UmdDB::inst()->getUids())
		{
			mResponseBuffer.push_back("NOTIFY_UID " + uid);
		}
		return true;
	}

	if (cmd == "QUERY")
	{
		QStringList uids = args.split(SPACE_REGEX);

		foreach(QString uid, uids)
		{
			bool exists;
			UmdDB::umdInfo umd = UmdDB::inst()->getUmd(uid, &exists);
			if (exists)
			{
				onUmdChanged(uid, UmdDB::allChanged, umd, true);
			}
			else
			{
				mResponseBuffer.push_back("ERROR UNKNOWN_UID " + uid);
			}
		}
		mResponseBuffer.push_back(".");
		return true;
	}
	if (cmd == "QUERY_ALL")
	{
		foreach(QString uid, UmdDB::inst()->getUids())
		{
			bool exists;
			UmdDB::umdInfo umd = UmdDB::inst()->getUmd(uid, &exists);
			if (exists)
			{
				onUmdChanged(uid, UmdDB::allChanged, umd, true);
			}
			else
			{
				mResponseBuffer.push_back("ERROR UNKNOWN_UID " + uid);
			}
		}
		return true;
	}
	if (cmd=="SUBSCRIBE")
	{
		QStringList arg_list = args.split(SPACE_REGEX);
		foreach(QString arg, arg_list)
		{
			if (!mSubscriptionList.contains(arg))
				mSubscriptionList.append(arg.toLower());
		}
		return true;
	}
	if (cmd=="UNSUBSCRIBE")
	{
		QStringList arg_list = args.split(SPACE_REGEX);
		foreach(QString arg, arg_list)
		{
			mSubscriptionList.removeAll(arg);
		}
		return true;
	}
	if (cmd=="SUBSCRIBE_ALL")
	{
		mSubscribeAll = true;
		return true;
	}
	if (cmd=="UNSUBSCRIBE_ALL")
	{
		mSubscribeAll = false;
		mSubscriptionList.clear();
		return true;
	}

	//	for(auto it = mBlockParameterMap.begin(); it!=mBlockParameterMap.end();++it)
	//		qDebug()<<"block paramter"<<it.key()<<it.value();
	mErrorString = "Invalid cmd";

	return false;
}

void simpleProtocol::registerHandler(QString cmd, cmdHandler handler)
{
	mHandlers[cmd.toUpper()] = handler;
}

void simpleProtocol::onReadyRead()
{
	if(!mStream)
		return;
	while (!mStream->atEnd())
	{
		mRxBuffer.push_back(mStream->readAll());
		mRxBuffer.replace("\r\n","\n");
		mRxBuffer.replace("\n\r","\n");
		int lineEnd = mRxBuffer.indexOf('\n');
		while(lineEnd>=0)
		{
			QString line = QString::fromUtf8(mRxBuffer.left(lineEnd));
			mRxBuffer = mRxBuffer.mid(lineEnd+1);
			lineEnd = mRxBuffer.indexOf('\n');
			bool ok = parseLine(line);
			bool inBlock = (mParserState == parserInBlock);
			if ((!inBlock) && (mResponseBuffer.size()))
			{
				foreach(QString s, mResponseBuffer)
				{
					QByteArray data = s.toUtf8();
					data.append('\n');
					mStream->write(data);
				}
				mResponseBuffer.clear();
			}
			if (ok)
			{
				if (!inBlock)
					mStream->write("OK\n.\n");
			}
			else
			{
				QByteArray data = QString("ERROR %1\n.\n")
				                  .arg(mErrorString).toUtf8();
				mStream->write(data);
			}
		}
	}
}

void simpleProtocol::onUmdChanged(QString uid, int changedMask, UmdDB::umdInfo umdData, bool force)
{
	if ((mSubscribeAll)||(mSubscriptionList.contains(uid.toLower()))||(force))
	{
		QString data;
		data += QString("UID %1\n").arg(uid);
		if (changedMask & UmdDB::tallyChanged)
		{
			data+= QString("TALLY");
			for (int i=0;i<8; ++i)
				if (umdData.tally & (1<<i))
					data+=QString::asprintf(" %d", i+1);
			data+="\n";
		}
		if (changedMask & UmdDB::umdTextChanged)
			data+= QString("UMD %1\n").arg(umdData.umd);
		if (changedMask & UmdDB::mnemonicChanged)
			data+= QString("MNEMONIC %1\n").arg(umdData.mnemonic);
		if (changedMask & UmdDB::textChanged)
			data+= QString("TEXT %1\n").arg(umdData.text);
		if (changedMask & UmdDB::propsChanged)
		{
			for(auto it = umdData.userProperties.begin();
			    it != umdData.userProperties.end(); ++it)
				data+= QString("PROP %1 %2\n").arg(it.key()).arg(it.value().toString());
		}
		data+=".\n";
		if (mStream)
			mStream->write(data.toUtf8());
		else
			qDebug()<<data;
	}
}


void simpleProtocol::setErrorString(QString e)
{
	mErrorString = e;
}
