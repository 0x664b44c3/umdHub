#include "umddatabase.h"
#include <QCoreApplication>
#include <QDebug>
#include <QJsonArray>
#include <QJsonValue>


UmdDB * UmdDB::mInst = nullptr;

UmdDB::UmdDB(QObject *parent) : QObject(parent)
{
	qDebug()<<"Created umd database inst";
	mTallyColors[0] = TALLY_COLOR1;
	mTallyColors[1] = TALLY_COLOR2;
	mTallyColors[2] = TALLY_COLOR3;
	mTallyColors[3] = TALLY_COLOR4;
	mTallyColors[4] = TALLY_COLOR5;
	mTallyColors[5] = TALLY_COLOR6;
	mTallyColors[6] = TALLY_COLOR7;
	mTallyColors[7] = TALLY_COLOR8;
}

UmdDB *UmdDB::inst()
{
	if (!mInst)
	{
		mInst = new UmdDB(QCoreApplication::instance());
		registerMetatypes();
	}
	return mInst;
}

void UmdDB::registerMetatypes()
{
	qRegisterMetaType<UmdDB::umdInfo>();
}

UmdDB::umdInfo UmdDB::getUmd(QString uid, bool *found) const
{
	if (mDatabase.contains(uid))
	{
		if (found)
			*found = true;
		return mDatabase.value(uid);
	}
	else
	{
		if (found)
			*found = false;
		return umdInfo();
	}
}

void UmdDB::reset()
{
	mDatabase.clear();
}

void UmdDB::removeUid(QString uid)
{
	if (!mDatabase.contains(uid))
		return;
	mDatabase.remove(uid);
	emit umdRemoved(uid);
}

QStringList UmdDB::getUids() const
{
	return mDatabase.keys();
}

bool UmdDB::exists(QString uid) const
{
	return mDatabase.contains(uid);
}

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
QJsonObject entry2json(QString uid, UmdDB::umdInfo umd)
{
	QJsonObject obj;
	obj.insert("uid", uid);
	obj.insert("umd", umd.umd);
	obj.insert("mnemonic", umd.mnemonic);
	obj.insert("text", umd.text);
	obj.insert("tally", umd.tally);
	obj.insert("direction", umd.direction);
	return obj;
}
QString color2html(quint32 color)
{
//	unsigned int alpha = (color >> 24) & 0xff;
	unsigned int red   = (color >> 16) & 0xff;
	unsigned int green = (color >>  8) & 0xff;
	unsigned int blue  = (color      ) & 0xff;
	return QString::asprintf("#%02x%02x%02x", red, green, blue);
}

QJsonObject UmdDB::getJson() const
{
	QJsonObject save;
	QJsonArray umdArray;
	for(auto it = mDatabase.begin(); it!=mDatabase.end(); ++it)
		umdArray.push_back(
		            entry2json(it.key(), it.value()));
	save.insert("umddb", umdArray);
	QJsonArray colorList;
	for(int i=0;i<8;++i)
		colorList.push_back(color2html(mTallyColors[i]));
	save.insert("colors", colorList);
	return save;
}

bool UmdDB::loadJson(const QJsonObject & data, bool reset)
{
	if (reset)
		mDatabase.clear();
	QJsonValue colors = data.value("colors");
	QJsonValue umdData = data.value("umddb");
	if (colors.isArray())
	{

	}

	if (umdData.isArray())
	foreach(QJsonValue jv, umdData.toArray())
	{
		if (!jv.isObject())
			continue;
		QJsonObject obj = jv.toObject();
		umdInfo umd;
		QString uid = obj.value("uid").toString();
		umd.umd = obj.value("umd").toString();
		umd.mnemonic = obj.value("mnemonic").toString();
		umd.text = obj.value("text").toString();
		umd.tally = obj.value("tally").toInt(0);
		umd.direction = obj.value("direction").toInt(0);
		setUmd(uid, umd);
	}
	return true;
}

void UmdDB::setUmd(QString uid, UmdDB::umdInfo info,
                         int updateMask)
{
	if (uid.isEmpty())
		return;
	int changes = allChanged;
	auto entry = mDatabase.find(uid);
	if (entry == mDatabase.end())
	{
		entry = mDatabase.insert(uid, info);
		updateMask = allChanged;
	}
	else
	{ //handle update
		if (info.tally == entry->tally)
			changes &= ~tallyChanged;
		if (info.umd   == entry->umd)
			changes &= ~umdTextChanged;
		if (info.mnemonic == entry->mnemonic)
			changes &= ~mnemonicChanged;
		if (info.text == entry->text)
			changes &= ~textChanged;
		if (info.direction == entry->direction)
			changes &= ~directionChanged;

		if (updateMask & tallyChanged)
			entry->tally = info.tally;
		if (updateMask & umdTextChanged)
			entry->umd  = info.umd;
		if (updateMask & mnemonicChanged)
			entry->mnemonic = info.mnemonic;
		if (updateMask & textChanged)
			entry->text = info.text;
		if (updateMask & directionChanged)
			entry->direction = info.direction;

		if (updateMask & propsChanged)
			entry->userProperties = info.userProperties;
		else
			changes &= ~propsChanged;
	}
	if (changes)
		emit umdChanged(uid, changes, entry.value());
}

void UmdDB::setTallyColor(int index, quint32 rgb)
{
	if ((index>7)||(index<0))
		return;
	mTallyColors[index] = rgb;
	emit tallyColorChanged(index, rgb);
}

