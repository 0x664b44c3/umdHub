#ifndef PACKETSCHEDULER_H
#define PACKETSCHEDULER_H

#include <QObject>
#include <QElapsedTimer>
#include <QMap>
class QIODevice;
class QTimer;
struct TslMessage
{
public:
	TslMessage(unsigned char addr, unsigned char tally=0, unsigned char bright=0)
	{
		this->address = addr;
		this->control = (tally & 0x0f)
		               |((bright &3) * 0x10);
	}
	unsigned char address;
	unsigned char control;
	QString text;
	unsigned char tally() const {return control & 0x0f;}
	unsigned char brightness() const {return (control & 0x30)>>4;}
	static unsigned char encodeControl(unsigned char tally, unsigned char brightness);
};

struct tslQueueStatus
{
	unsigned char brigthness;
	unsigned char tally;

	QString text;
	bool needsUpdate;
	QElapsedTimer retransmitTimer;
};


class QUdpSocket;
class TslTransmitter : public QObject
{
	Q_OBJECT
public:
	explicit TslTransmitter(QObject *parent = nullptr);
	QString getUmd(int id, bool *ok=0, int* tally=0, int* brightness=0) const;

	void reset();
	int mtu() const;
	void setMTU(int mtu);
	void flushData(bool periodic);
	void setStreamDevice(QIODevice *, int maxWriteQueue=0);

	int refreshInterval() const;
	void setRefreshInterval(int refreshInterval);

	/** true if TSL line mode is enabled (rs-485/tcp mode only) */
	bool isTslMode() const;

	/** Enable/disable TSL line mode
	 * Enable for maximum compatibility with older equipment.
	 * In TSL line mode the behaviour of the original equipment is replicated:
	 * Cyclic umd messages are transmitted every 200ms with async updates
	 * when required.
	 * A fully utilized Buss will result in a full refresh every 25s.
	 *
	 * Otherwise all channels will be repeated in priorized by the
	 * time since their last transmission. Pending async updates will
	 * always be priorized to cyclic updates at any time.
	 * A fully utilized bus will result in a full refresh
	 * approximately every refreshInterval
	**/
	void setTslMode(bool tslMode);

	int maxWriteQueue() const;

	/** set the maximum write queue size in stream mode
	 * A sensible setting is
	 * (bit_rate / wire_char_size) / 10 * 3 / 4
	 * where /10 comes from the refresh timer rate of 100ms
	 *
	 * On a typical TSL serial link with 8E1 caharcter format
	 * one byte is transmitted as 1 start bit, 8 data bits, 1 parity and 1 stop
	 * giving 11 bits on the wire
	 *
	 * 38400 8E1 would translate to
	 * 38400 / 11 / 10 * 3/4 = 261 Bytes max buffer fill
	 *
	 * this ensures that 1/4 of the bandwidth is preserved at any time
	 * for async packets and ensures the buffer will flushed
	 * by the next timer tick.
	 *
	 * Recommended value for 38400 8E1 is 240..261
	 *
	 * NOTE: This setting is irrelevant for TSL line mode where
	 * a different scheduling approach is taken.
	 */
	void setMaxWriteQueue(int maxWriteQueue);

signals:

	void streamError(QString);
private slots:
	void onTimer();

public slots:
	void setUmd(int addr, QString text, unsigned char tally = 0, unsigned char brightness=3);
	void removeUmd(unsigned char addr);
	void transmitNow();
	void onChannelError();

private:
	int mMTU;
	int mRefreshInterval;
	int mTally;
	int mConnectionType;
	int mMaxWriteQueue;
	int mBitrateAccu;
	bool mTslMode;
	QTimer * mRefreshTimer;
	QUdpSocket * mSocket;
	QIODevice * mStream;
	QMap<int, tslQueueStatus> mStatusBuffer;
};

#endif // PACKETSCHEDULER_H
