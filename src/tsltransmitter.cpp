#include "tsltransmitter.h"
#include <tsl.h>
#include <QUdpSocket>
#include <QTimer>
#include <QDateTime>
#include <QIODevice>
#include <QDebug>
#include <algorithm>
enum {
	connUndefined=-1,
	connUDP=0,
	connStream=1
};

TslTransmitter::TslTransmitter(QObject *parent) :
    QObject(parent), mMTU(1024), mRefreshInterval(1000), mMaxWriteQueue(256),
    mTslMode(false)
{
	mSocket = new QUdpSocket(this);
	mRefreshTimer = new QTimer(this);
	mRefreshTimer->start(100);
	connect(mRefreshTimer , SIGNAL(timeout()), SLOT(onTimer()));
	mTally = 0;
	mStream = nullptr;
	mConnectionType = connUDP;
}

QString TslTransmitter::getUmd(int id, bool *ok, int *tally, int *brightness) const
{
	if (!mStatusBuffer.contains(id))
	{
		if (ok)
			*ok=false;
		if (tally)
			*tally=0;
		if (brightness)
			*brightness=0;
		return QString();
	}
	auto entry = mStatusBuffer[id];
	if (tally)
		*tally=entry.tally;
	if (brightness)
		*brightness=entry.brigthness;
	if (ok)
		*ok=true;
	return entry.text;
}

void TslTransmitter::reset()
{
	mStatusBuffer.clear();
}

void TslTransmitter::setUmd(int addr, QString text, unsigned char tally, unsigned char brightness)
{
	if (addr>127)
		return;
	if (addr<0)
		return;
	tslQueueStatus *entry=nullptr;
	if (!mStatusBuffer.contains(addr))
	{
		struct tslQueueStatus qs;
		qs.retransmitTimer.restart();
		mStatusBuffer.insert(addr, qs);
	}
	entry = &mStatusBuffer[addr];
	entry->tally = tally;
	entry->brigthness = brightness;
	entry->text = text;
	entry->needsUpdate = true;
}

void TslTransmitter::removeUmd(unsigned char addr)
{
	mStatusBuffer.remove(addr);
}

void TslTransmitter::onTimer()
{
	//	 setUmd(1, QDateTime::currentDateTime().toString("hh:mm:ss"));
	if (mMaxWriteQueue)
		mBitrateAccu = qMax(0, mBitrateAccu - mMaxWriteQueue / 10);
	flushData(true);
}

void TslTransmitter::transmitNow()
{
	flushData(false);
}

void TslTransmitter::onChannelError()
{
	mStream->close();
	emit streamError(mStream->errorString());
	mStream->deleteLater();
	mStream = nullptr;
}

int TslTransmitter::maxWriteQueue() const
{
	return mMaxWriteQueue;
}

void TslTransmitter::setMaxWriteQueue(int maxWriteQueue)
{
	mMaxWriteQueue = maxWriteQueue;
}

bool TslTransmitter::isTslMode() const
{
	return mTslMode;
}

void TslTransmitter::setTslMode(bool tslMode)
{
	mTslMode = tslMode;
	if (mTslMode)
		mRefreshTimer->setInterval(200);
	else
		mRefreshTimer->setInterval(100);
}

int TslTransmitter::refreshInterval() const
{
	return mRefreshInterval;
}

void TslTransmitter::setRefreshInterval(int refreshInterval)
{
	mRefreshInterval = qMax(500, refreshInterval);
}

int TslTransmitter::mtu() const
{
	return mMTU;
}

void TslTransmitter::setMTU(int mTU)
{
	mMTU = mTU;
}

struct prioData {int id; int prio;};
bool compPrio(const prioData & a, const prioData & b)
{
	return a.prio < b.prio;
}
void TslTransmitter::flushData(bool periodic)
{

	//build priority queue for this transmit interval
	QList<int> updateList;
	QList<prioData> periodicList;
	for(auto it = mStatusBuffer.begin(); it!=mStatusBuffer.end(); ++it)
	{

		if (
		        (it->needsUpdate)
		        //only force-send periodic updates if we are not force-flushing the queue
		        || ((mMaxWriteQueue==0) && (periodic && (it->retransmitTimer.elapsed()>mRefreshInterval))))
		{
			updateList.push_back(it.key());
		}
		else
		{
			//if this is a forced update, fill the frame using half the refresh timeout
			if (it->retransmitTimer.elapsed()>(periodic?mRefreshInterval:mRefreshInterval/2))
			{
				prioData pp;
				pp.id = it.key();
				pp.prio = mRefreshInterval - it->retransmitTimer.elapsed();
				periodicList.push_back(pp);
			}
		}
	}
	std::sort(periodicList.begin(), periodicList.end(), [](const prioData & a, const prioData & b){return a.prio < b.prio;});


	QByteArray buffer;
	if (mConnectionType == connUDP)
	{
		/* if we are using UDP
		we need to transmit updatelist and
		can fill up the packet using periodicList
		*/

		bool packetWasFull=false;
		foreach(int i, updateList)
		{
			packetWasFull = false;
			buffer.push_back(TSL::assembleMessage(
			                     i,
			                     mStatusBuffer[i].text,
			                     mStatusBuffer[i].tally,
			                     mStatusBuffer[i].brigthness
			                     ));
			mStatusBuffer[i].needsUpdate = false;
			mStatusBuffer[i].retransmitTimer.restart();
			if (buffer.size()>=mMTU)
			{
				mSocket->writeDatagram(buffer, QHostAddress::Broadcast, 1234);
				buffer.clear();
				packetWasFull = true;
			}
		}
		if (!packetWasFull)
		{
			foreach(prioData pe, periodicList)
			{
				int i=pe.id;
				buffer.push_back(TSL::assembleMessage(
				                     i,
				                     mStatusBuffer[i].text,
				                     mStatusBuffer[i].tally,
				                     mStatusBuffer[i].brigthness
				                     ));
				mStatusBuffer[i].needsUpdate = false;
				mStatusBuffer[i].retransmitTimer.restart();
				if (buffer.size()>=mMTU)
					break;
			}
			if (buffer.size())
			{
				mSocket->writeDatagram(buffer, QHostAddress::Broadcast, 1234);
			}
		}
	}
	if (mConnectionType == connStream)
	{
		if (!mStream)
			return;
		if (!mStream->isWritable())
			qCritical()<<"Cannot write to stream.";

		int msgSent=0;
		//forced updates take precedence
		foreach(int i, updateList)
		{
			buffer = TSL::assembleMessage(
			             i,
			             mStatusBuffer[i].text,
			             mStatusBuffer[i].tally,
			             mStatusBuffer[i].brigthness
			             );
			mStatusBuffer[i].needsUpdate = false;
			mStatusBuffer[i].retransmitTimer.restart();
			mStream->write(buffer);
			buffer.clear();
			++msgSent;
		}
		foreach(prioData pe, periodicList)
		{
			int i = pe.id;
			if ((mTslMode) && (msgSent>0))
				break;
			if ((mMaxWriteQueue) && (mStream->bytesToWrite() > mMaxWriteQueue))
			{
				break;
			}

			buffer = TSL::assembleMessage(
			             i,
			             mStatusBuffer[i].text,
			             mStatusBuffer[i].tally,
			             mStatusBuffer[i].brigthness
			             );
			mStatusBuffer[i].needsUpdate = false;
			mStatusBuffer[i].retransmitTimer.restart();
			mStream->write(buffer);
			buffer.clear();
			++msgSent;
		}

	}
}

void TslTransmitter::setStreamDevice(QIODevice * dev, int maxWriteQueue)
{
	mConnectionType = connStream;
	mStream = dev;
	mMaxWriteQueue = maxWriteQueue;
}

unsigned char TslMessage::encodeControl(unsigned char tally, unsigned char brightness)
{
	return (tally & 0x0f) | ((brightness&3)*0x10);
}
