#ifndef PROTOCOLLISTENER_H
#define PROTOCOLLISTENER_H

#include <QObject>

class QTcpServer;
class ProtocolListener : public QObject
{
	Q_OBJECT
public:
	explicit ProtocolListener(QObject *parent = nullptr);

private:
	QTcpServer * mListener;
signals:

private slots:
	void onConnection();
public slots:
};

#endif // PROTOCOLLISTENER_H
