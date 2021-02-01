#ifndef SIMPLEPROTOCOL_H
#define SIMPLEPROTOCOL_H

/** protocol description
 *
 * line oriented text protocol; a double break (\\n\\n or \\r\\r)
 * denotes end of a block of information. CRs will be translated to LF
 * characters upon reception except when directly next to a LF
 *
 * character encoding: utf-8
 *
 * Lines are in the format
 * KEYWORD [argument]\n
 *
 * a block may be terminated with ".\n" which has the same effect as
 * a double linebreak.
 *
 * Normal tally information blocks start with the field UID
 * (which mus be the first line in the block)
 * UID <unique-id>
 * specifying a unique port id which can be
 * referenced in tally mapping files.
 *
 * optional keywords are
 * TALLY [numbers 1 to 8, separated by spaces]

 * MNEMONIC <string> (usually 4 chars)
 * UMD <string> (usually 8 chars)
 * NAME <string> (any length of text)
 * DIRECTION [source, sink]
 *
 * the UID and TALLY directive MUST be implemented by all implementations
 *
 * additionally the following commands may be implemented
 *
 * subscribe to a uid's status changes (to keep traffic low in large systems)
 * SUBSCRIBE <uid>
 * SUBSCRIBE_ALL
 *
 * unsubscribe to a uid's status changes
 * UNSUBSCRIBE <uid>
 * UNSUBSCRIBE_ALL
 *
 * list all UIDs known to the server
 * LIST
 *
 * queries the status of an UID
 * QUERY <UID>
 *
 * generic info block that may contain status and verison info
 * INFO
 * [lines in the generic KEYWORD <value> format]
 * .
 *
 * ERROR <text>
 * sent if a command could not be parsed
 *
 * if you want to cancel a line and reset the parser, send <BS><LF>
 * if you want to cancel a block send <BS><BS><LF>
 *
 * a line that starts with a semicolon (;) is a comment
 * or may be used to transmit large blocks of text
 *
**/

#include <QObject>
#include <QStringList>
#include <QIODevice>
#include <QMap>
#include <umddatabase.h>
#include <QHash>
class simpleProtocol;
typedef bool(*cmdHandler)(simpleProtocol*, QString, QString);

class simpleProtocol : public QObject
{
	Q_OBJECT
public:
	explicit simpleProtocol(QObject *parent = nullptr);
	~simpleProtocol();
	bool parseLine(QString line);
	void setStream(QIODevice * dev);
	QStringList lastResonse() const;
	QString errorString() const;
	QIODevice * stream() const;
	void addResponse(QString);
	QMap<QString, QString> blockParameters() const;
	void setErrorString(QString e);
signals:
	void tallyChanged(QString uid, int tallies);
	void umdChanged(QString uid, int tallies);
	void mnemonicChanged(QString uid, QString mnemonic);

private:
	QStringList mSubscriptionList;
	QMap<QString, QString> mBlockParameterMap;
	QString mCurrentBlockCmd;
	QString mCurrentBlockCmdArgs;
	QString lastCmd;
	int mParserState;
	QIODevice *mStream;
	QStringList mBlockCommands;
	bool processCommand(QString cmd, QString args);
	QStringList mResponseBuffer;
	QString mErrorString;
	QByteArray mRxBuffer;
	void registerHandler(QString cmd, cmdHandler);
	QHash<QString, cmdHandler> mHandlers;
	bool mSubscribeAll;
signals:
private slots:
	void onReadyRead();
public slots:
	void onUmdChanged(QString uid, int changedMask, UmdDB::umdInfo properties, bool force = false);
};

#endif // SIMPLEPROTOCOL_H
