#include "umdmapper.h"
#include <QDebug>
#include <QJsonObject>
#define LOG_WARN qWarning()<<__FILE__<<":"<<__LINE__
const int relevantChanges = UmdDB::tallyChanged
                            | UmdDB::umdTextChanged
                            | UmdDB::mnemonicChanged
                            | UmdDB::textChanged ;
UmdMapper::UmdMapper(QObject *parent) : QObject(parent)
{

}

bool UmdMapper::channelMapped(int channel) const
{
	for(auto it = mMappings.begin(); it!=mMappings.end(); ++it)
	{
		if (it->tslChannel == channel)
			return true;
	}
	return false;
}

//a mapping to an uid can only occour several times in the list
void UmdMapper::deleteMappings(const QString &uid)
{
	for(auto it = mMappings.begin(); it!=mMappings.end();)
	{
		if (it.key() == uid)
		{
			it = mMappings.erase(it);
			continue;
		}
		++it;
	}
}

//a mapping to a channel can only occour once in the list
void UmdMapper::deleteMapping(int channel)
{
	for(auto it = mMappings.begin(); it!=mMappings.end(); ++it)
	{
		if (it->tslChannel == channel)
		{
			it = mMappings.erase(it);
			break;
		}
	}
}

void UmdMapper::setChannel(TslTransmitter *xmitter)
{
	mTsl = xmitter;
}

bool UmdMapper::setMapping(QString uid,
                           int channel,
                           QString umdTemplate,
                           QVector<int> tallyMap)
{
	if ((channel<0)||(channel>126))
		return false;
	Q_UNUSED(umdTemplate)
	Q_UNUSED(tallyMap)
	if (channelMapped(channel))
		return false;
	umdMapping mapping;
	mapping.tslChannel = channel;
	mMappings.insert(uid, mapping);
	if (!mTsl)
		return true;
	bool found=false;
	UmdDB::umdInfo info = UmdDB::inst()->getUmd(uid, &found);
	QString umdText = found?info.umd:QString("TSL %1").arg(channel);
	mTsl->setUmd(channel, umdText);
	return true;
}

//FIXME: tally map is ignored atm
bool UmdMapper::initFromJson(const QJsonArray &mappings)
{
	mMappings.clear();

	if (mTsl)
		mTsl->reset();

	foreach(QJsonValue v, mappings)
	{
		if (!v.isObject())
		{
			LOG_WARN <<"Skipping array entry that is not an object";
			continue;
		}
		QJsonObject mapObj = v.toObject();
		QJsonValue umdId = mapObj.value("umd_id");
		QJsonValue UMD = mapObj.value("umd");
		QJsonValue map = mapObj.value("map");

		if (
		        (umdId.isUndefined()) ||
		        (map.isUndefined()) ||
		        (map.toString().isEmpty())
		        )
		{
			LOG_WARN << "Incomplete data for object";
			continue;
		}
		int id = umdId.toInt(-1);
		if (id<0)
			continue;
		setMapping(map.toString(), id, UMD.toString());
	}
	return true;
}

QJsonArray UmdMapper::getJson() const
{
	QJsonArray array;
	for(auto it = mMappings.begin(); it!=mMappings.end();)
	{
		QJsonObject mapping;
		mapping.insert("map", it.key());
		mapping.insert("umd_id", it->tslChannel);
		if (it->tallyMap.size())
		{
			QJsonArray tmap;
			foreach(int m, it->tallyMap)
			{
				tmap.push_back(QJsonValue(m));
			}
			mapping.insert("tally_map", tmap);
		}
		if (it->umdTemplate.size())
			mapping.insert("umd", it->umdTemplate);
		array.push_back(mapping);
		++it;
	}
	return array;
}

void UmdMapper::onUmdChanged(QString uid, int changedMask, UmdDB::umdInfo umdData)
{
	if(!mTsl)
		return;
	if ((changedMask & relevantChanges) == 0)
		return;
	QList<umdMapping> mappings = mMappings.values(uid);
	if  (mappings.isEmpty())
		return;
	foreach(auto mapping, mappings)
	{
		int tally=0;
		int brightness = 255;
		if (mapping.tallyMap.size() == 0)
		{
			//just map all four tallies into the tally bits
			for(int i=0; i < 4; ++i)
			{
				if (umdData.tally & (1<<(i)))
					tally|=1<<i;
			}
		}
		else
		{
			for(int i=0; i < mapping.tallyMap.size(); ++i)
			{
				int sourceTally = mapping.tallyMap[i];
				if (i>3)
					break;
				if (umdData.tally & (1<<(sourceTally-1)))
					tally|=1<<i;
			}
		}
		QString text = umdData.umd;
		mTsl->setUmd(mapping.tslChannel, text, tally, (brightness>>6)&3);
	}
}
