#ifndef Server_h
#define Server_h

#ifdef ENABLE_SERVER
#include <QtNetwork>

class Server : public QTcpServer
{
    Q_OBJECT
public:
    ~Server();
    static Server *instance();
    static int port();
    bool listen() { return QTcpServer::listen(QHostAddress::LocalHost, port()); }
    static bool write(const QString &command);
protected:
    void incomingConnection(int handle);
private slots:
    void onReadyRead();
signals:
    void commandReceived(const QString &command);
private:
    Server();
};

#endif
#endif
