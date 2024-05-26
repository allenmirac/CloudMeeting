#ifndef MYTCPSOCKET_H
#define MYTCPSOCKET_H

#include <QThread>
#include <QTcpSocket>
#include <QMutex>
#include <QMetaObject>
#include <QHostAddress>
#include <QMutexLocker>
#include <QtEndian>
#include <QByteArray>
#include "netheader.h"
#include "logger.h"
#include "json.h"
using json = nlohmann::json;

class MyTcpSocket : public QThread
{
    Q_OBJECT
public:
    MyTcpSocket(QObject* parent=nullptr);
    ~MyTcpSocket();
    bool connectToServer(QString, QString, QIODevice::OpenModeFlag);
    QString errorString();
    void disconnectFromHost();
    quint32 getLocalIp();

private:
    void run() override;
    qint64 readn(char *, quint64, int);
    QTcpSocket *m_socktcp;
    QThread *m_sockThread;
    QByteArray m_sendbuf;
    QByteArray m_recvbuf;
    quint64 m_hasReceive;

    QMutex m_lock;
    volatile bool m_isCanRun;

private slots:
    bool connectServer(QString, QString, QIODevice::OpenModeFlag);
    void sendData(json*);
    void closeSocket();

public slots:
    void recvFromSocket();
    void stopImmediately();
    void errorDetect(QAbstractSocket::SocketError error);
signals:
    void sockerror(QAbstractSocket::SocketError error);
    void sendTextOver();
};

#endif // MYTCPSOCKET_H
