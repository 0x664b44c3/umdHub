#ifndef UMDDATABASE_H
#define UMDDATABASE_H
#include <QVariantMap>
#include <QObject>
#include <QHash>

#include <QJsonObject>

/** default color mappings of up to 8 tally flags
 * these are configured per system and can be
 * implemented for protocols that support them
**/
#define TALLY_COLOR1 0xffff0000
#define TALLY_COLOR2 0xff00ff00
#define TALLY_COLOR3 0xffff0000
#define TALLY_COLOR4 0xff00ff00
//remaining 4 are amber for now
#define TALLY_COLOR5 0xffffa000
#define TALLY_COLOR6 0xffffa000
#define TALLY_COLOR7 0xffffa000
#define TALLY_COLOR8 0xffffa000


class UmdDB : public QObject
{
	Q_OBJECT
public:

	enum changeFlags
	{
		tallyChanged     =0x0001,
		umdTextChanged   =0x0002,
		mnemonicChanged  =0x0004,
		textChanged      =0x0008,
		directionChanged =0x0010,
//		colorsChanged    =0x0020,
		propsChanged     =0x0100,
		allChanged       =0x01ff
	};
	static UmdDB * inst();
	QByteArray dumpJson() const;

	enum
	{
		dirSource=0,
		dirSink=1
	};
	struct umdInfo
	{
		QString umd;
		QString mnemonic;
		QString text;
		int tally;
		int direction;
		QVariantMap userProperties;
//		quint32 tallyColors[8];
	};


	static void registerMetatypes();
	umdInfo getUmd(QString uid, bool * found=0) const;
	void reset();
	void removeUid(QString uid);
	QStringList getUids() const;
	bool exists(QString uid) const;
	QJsonObject getJson() const;
	bool loadJson(const QJsonObject &, bool reset=true);

signals:
	void umdChanged(QString uid, int changedMask, UmdDB::umdInfo properties);
	void umdRemoved(QString uid);
	void tallyColorChanged(int index, quint32 rgb);
public slots:
	void setUmd(QString uid, UmdDB::umdInfo info, int updateMask = allChanged);
	void setTallyColor(int index, quint32 rgb);
private:
	explicit UmdDB(QObject *parent = nullptr);
	static UmdDB * mInst;
	UmdDB(const UmdDB &, QObject*o);
	QHash<QString, umdInfo> mDatabase;
	quint32 mTallyColors[8];
};

Q_DECLARE_METATYPE(UmdDB::umdInfo)

#endif // UMDDATABASE_H
