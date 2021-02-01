#include "protocollistener.h"
#include "simpleprotocol.h"
#include <QTcpServer>
#include <QTcpSocket>
#include <QDebug>
#include "umddatabase.h"
ProtocolListener::ProtocolListener(QObject *parent) :
    QObject(parent), mListener(new QTcpServer(this))
{
	mListener->listen(QHostAddress::Any, 8433);
	connect(mListener, SIGNAL(newConnection()), SLOT(onConnection()));
}

void ProtocolListener::onConnection()
{
	while(mListener->hasPendingConnections())
	{
		QTcpSocket * socket = mListener->nextPendingConnection();
		qDebug()<<"Accepting connection from"<<socket->peerAddress();
		simpleProtocol * protHandler = new simpleProtocol(this);
		protHandler->setStream(socket);
		connect(socket, SIGNAL(disconnected()), protHandler, SLOT(deleteLater()));
		connect(UmdDB::inst(), SIGNAL(umdChanged(QString,int,UmdDB::umdInfo)),
		        protHandler, SLOT(onUmdChanged(QString,int,UmdDB::umdInfo)));

	}
}
