#include "Server.h"
#include "Config.h"
#include <QtGui>

static Server *sInstance = 0;

Server::Server()
    : QTcpServer(qApp)
{
    Q_ASSERT(!sInstance);
    sInstance = this;
}

Server::~Server()
{
    Q_ASSERT(sInstance == this);
    sInstance = 0;
}

Server * Server::instance()
{
    if (!sInstance)
        new Server;
    return sInstance;
}
void Server::onReadyRead()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    Q_ASSERT(socket);
    QVariant variant = socket->property("pending");
    qint32 pending;
    if (variant.isNull()) {
        if (socket->bytesAvailable() < int(sizeof(qint32))) {
            return;
        }
        QDataStream ds(socket);
        ds >> pending;
        if (socket->bytesAvailable() < pending) {
            socket->setProperty("pending", pending);
            return;
        }
    } else {
        pending = variant.toInt();
    }
    if (socket->bytesAvailable() >= pending) {
        QString command;
        QDataStream ds(socket);
        ds >> command;
        emit commandReceived(command);
        delete socket;
    }
}

int Server::port()
{
    const static int port = Config().value<int>("serverPort", 8899);
    return port;
}

void Server::incomingConnection(int sock)
{
    QTcpSocket *socket = new QTcpSocket(this);
    socket->setSocketDescriptor(sock);
    connect(socket, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
}
bool Server::write(const QString &command)
{
    QTcpSocket socket;
    socket.connectToHost(QHostAddress::LocalHost, Server::port(), QIODevice::WriteOnly);
    Config config;
    if (!socket.waitForConnected(config.value<int>("serverConnectionTimeout", 3000))) {
        qWarning("Can't seem to connect to server");
        return false;
    }
    QByteArray data;
    {
        QDataStream ds(&data, QIODevice::WriteOnly);
        ds << qint32(data.size());
        ds << command;
        ds.device()->seek(0);
        ds << qint32(data.size() - sizeof(qint32));
    }
    socket.write(data);
    if (!socket.waitForBytesWritten(config.value<int>("serverWriteTimeout", 3000))) {
        qWarning("Can't seem to write to server");
        return 1;
    }
    return 0;    
}
