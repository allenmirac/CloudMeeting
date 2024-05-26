#include "mytcpsocket.h"

extern MSG_QUEUE<json> send_queue;
extern MSG_QUEUE<json> recv_queue;
extern MSG_QUEUE<json> recv_audioQueue;

MyTcpSocket::MyTcpSocket(QObject* parent) : QThread(parent)
{
    qRegisterMetaType<QAbstractSocket::SocketError>();
    m_socktcp=nullptr;

    m_sockThread=new QThread();
    this->moveToThread(m_sockThread);
    connect(m_sockThread, SIGNAL(finished()), this, SLOT(closeSocket()));


    m_hasReceive = 0;
}

MyTcpSocket::~MyTcpSocket()
{
    m_sendbuf.clear();
    m_recvbuf.clear();
}

bool MyTcpSocket::connectToServer(QString ip, QString port, QIODevice::OpenModeFlag flag)
{
    m_sockThread->start();
    bool ret;
    //  跨线程执行一个槽函数
    //  表示在调用 connectServer 槽函数时，当前线程将被阻塞，直到槽函数执行完成并返回结果。
    //  connectServer接受三个参数
    QMetaObject::invokeMethod(this, "connectServer", Qt::BlockingQueuedConnection, Q_RETURN_ARG(bool, ret),
                              Q_ARG(QString, ip), Q_ARG(QString, port), Q_ARG(QIODevice::OpenModeFlag, flag));

    if(ret){
        this->start();
        return true;
    }else{
        return false;
    }
}

QString MyTcpSocket::errorString()
{
    return m_socktcp->errorString();
}

void MyTcpSocket::disconnectFromHost()
{
    if(this->isRunning()) {
        QMutexLocker locker(&m_lock);
        m_isCanRun = false;
    }
    if(m_sockThread->isRunning()) {
        m_sockThread->quit();
        m_sockThread->wait();
    }
}

quint32 MyTcpSocket::getLocalIp()
{
    if(m_socktcp->isOpen()) {
        return m_socktcp->localAddress().toIPv4Address();
    }else {
        return -1;
    }
}

void MyTcpSocket::run()
{
    m_isCanRun = true;
    /*
     *发送数据的结构
     *$_MSGType_IPV4_MSGSize_data_# //
     * 1 2 4 4 MSGSize 1
     *底层写数据线程
     */
    for(;;) {
        {
            QMutexLocker lock(&m_lock);
            if (!m_isCanRun) return;
        }

        json* send = nullptr;
        {
            QMutexLocker lock(&m_lock);
            send = send_queue.pop_msg();
        }

        if (send != nullptr) {
            LOG_DEBUG << "send queue pop msg";
            QMetaObject::invokeMethod(this, "sendData", Q_ARG(json*, send));
        }
    }
}

qint64 MyTcpSocket::readn(char *buf, quint64 maxSize, int n)
{
    quint64 hasToRead = n;
    quint64 hasRead = 0;
    do{
        qint64 ret = m_socktcp->read(buf+hasRead, hasToRead);
        if(ret < 0){
            return -1;
        }
        if(ret == 0){
            return hasRead;
        }
        hasRead += ret;
        hasToRead -= ret;
    }while(hasToRead>0 && hasRead<maxSize);
    return hasRead;
}

bool MyTcpSocket::connectServer(QString ip, QString port, QIODevice::OpenModeFlag flag)
{
    if(m_socktcp == nullptr) m_socktcp = new QTcpSocket();
    LOG_DEBUG << ip << "," << port;
    m_socktcp->connectToHost(ip, port.toUShort(), flag);
    // 最后一个参数保证连接到槽不会重复连接
    connect(m_socktcp, SIGNAL(readyRead()), this, SLOT(recvFromSocket()), Qt::UniqueConnection);
    if(m_socktcp->waitForConnected(50000)){
        return true;
    }
    m_socktcp->close();
    return false;
}

void MyTcpSocket::sendData(json* sendJson)
{
    if(m_socktcp->state() == QAbstractSocket::UnconnectedState){
        (*sendJson).clear();
        return;
    }

    // write to dets in big-endian byte order.
    //构造消息头
    /*m_sendbuf[bytesToWrite++] = '$';

    //消息类型
    qToBigEndian<quint16>(send->msg_type, m_sendbuf + bytesToWrite);
    bytesToWrite += 2;

    //发送者ip
    quint32 ip = m_socktcp->localAddress().toIPv4Address();
    qToBigEndian<quint32>(ip, m_sendbuf + bytesToWrite);
    bytesToWrite += 4;
    // 创建会议 发送音频 关闭摄像头 发送图片 发送文字
    // 将msg_len写入
    if (send->msg_type == CREATE_MEETING || send->msg_type == AUDIO_SEND
            || send->msg_type == CLOSE_CAMERA || send->msg_type == IMG_SEND
            || send->msg_type == TEXT_SEND) {
        //发送数据大小
        qToBigEndian<quint32>(send->msg_len, m_sendbuf + bytesToWrite);
        bytesToWrite += 4;//有点没看明白为什么+4 只将len写入
    }else if(send->msg_type == JOIN_MEETING)
    {
        qToBigEndian<quint32>(send->msg_len, m_sendbuf + bytesToWrite);
        bytesToWrite += 4;
        uint32_t room;
        memcpy(&room, send->msg_data, send->msg_len);
        qToBigEndian<quint32>(room, send->msg_data);
    }
    // 消息写入
    memcpy(m_sendbuf + bytesToWrite, send->msg_data, send->msg_len);
    bytesToWrite += send->msg_len;
    // 消息头部信息的尾部
    m_sendbuf[bytesToWrite++] = '#'; //结尾字符
    */
    LOG_DEBUG << "send data";
    if(sendJson==nullptr) {
        LOG_DEBUG << "sendJson ==nullptr";
    }
    m_sendbuf = QByteArray::fromStdString((*sendJson).dump());

    qint64 bytesToWrite = m_sendbuf.size();
    qint64 hasWritten = 0;
    while (hasWritten < bytesToWrite) {
        qint64 ret = m_socktcp->write(m_sendbuf.constData() + hasWritten, bytesToWrite - hasWritten);
        if (ret == -1) {
            if (m_socktcp->error() == QAbstractSocket::TemporaryError) {
                ret = 0;
            } else {
                LOG_WARN << "network error";
                break;
            }
        }
        hasWritten += ret;
    }

    m_socktcp->waitForBytesWritten();
    LOG_DEBUG << "sendData " << bytesToWrite << " bytes";
    int msg_type = (*sendJson)["msgType"];
    if(msg_type == TEXT_SEND){
        emit sendTextOver();
    }

}

void MyTcpSocket::closeSocket()
{
    if(m_socktcp && m_socktcp->isOpen()){
        m_socktcp->close();
    }
}

void MyTcpSocket::recvFromSocket()
{
    m_recvbuf  = m_socktcp->readAll();
    // 数据包不够
    if (m_recvbuf.size() == 0) {
        LOG_WARN << "error or no more data";
        return;
    }else{
        json* recvJson = new json(json::parse(m_recvbuf.constData()));
        int msgtype = (*recvJson)["msgType"];
        if (msgtype == CREATE_MEETING_RESPONSE || msgtype == JOIN_MEETING_RESPONSE
                || msgtype == PARTNER_JOIN2 || msgtype==BROAD_ADDUSER_MESSAGE){
            if (msgtype == CREATE_MEETING_RESPONSE) {
                recv_queue.push_msg(recvJson);
            }else if(msgtype == JOIN_MEETING_RESPONSE) {
                recv_queue.push_msg(recvJson);
            }else if(msgtype == PARTNER_JOIN2){ // 加入多个人
                recv_queue.push_msg(recvJson);
            }
            else if(msgtype == BROAD_ADDUSER_MESSAGE)
            {
                recv_queue.push_msg(recvJson);
            }
        }else if (msgtype == IMG_RECV || msgtype == PARTNER_JOIN
                  || msgtype == PARTNER_EXIT || msgtype == AUDIO_RECV
                  || msgtype == CLOSE_CAMERA || msgtype == TEXT_RECV) {
            //            quint32 ip;
            //            qFromBigEndian<quint32>(m_recvbuf + 3, 4, &ip);
            if (msgtype == IMG_RECV) {
                std::string data = (*recvJson).at("data").get<std::string>();
                QByteArray byteArray(data.c_str(), data.length());
                QByteArray rc = QByteArray::fromBase64(byteArray);
                QByteArray rdc = qUncompress(rc);
                (*recvJson).at("data") = rdc.toStdString();
                recv_queue.push_msg(recvJson);
                //                QByteArray cc((char *) m_recvbuf + MSG_HEADER, data_size);
                //                QByteArray rc = QByteArray::fromBase64(cc);
                //                QByteArray rdc = qUncompress(rc);//解压缩
                //                //将消息加入到接收队列
                //                if (rdc.size() > 0)
                //                {
                //                    MESSAGE* msg = (MESSAGE*)malloc(sizeof(MESSAGE));
                //                    if (msg == nullptr) {
                //                        LOG_DEBUG << " malloc failed";
                //                    } else {
                //                        memset(msg, 0, sizeof(MESSAGE));
                //                        msg->msg_type = msgtype;
                //                        msg->msg_data = (uchar*)malloc(rdc.size()); // 10 = format + width + width
                //                        if (msg->msg_data == NULL) {
                //                            free(msg);
                //                            LOG_DEBUG << " malloc failed";
                //                        } else {
                //                            memset(msg->msg_data, 0, rdc.size());
                //                            memcpy_s(msg->msg_data, rdc.size(), rdc.data(), rdc.size());
                //                            msg->msg_len = rdc.size();
                //                            msg->ip = ip;
                //                            recv_queue.push_msg(msg);
                //                        }
                //                    }
                //                }
            } else if (msgtype == PARTNER_JOIN || msgtype == PARTNER_EXIT || msgtype == CLOSE_CAMERA) { //加入一个人
                recv_queue.push_msg(recvJson);
            } else if (msgtype == AUDIO_RECV) { // 记得输入数据长度
                std::string data = (*recvJson).at("data").get<std::string>();
                QByteArray byteArray(data.c_str(), data.length());
                QByteArray rc = QByteArray::fromBase64(byteArray);
                QByteArray rdc = qUncompress(rc);
                (*recvJson).at("data") = rdc.toStdString();

                recv_audioQueue.push_msg(recvJson);
            } else if(msgtype == TEXT_RECV) {
                recv_queue.push_msg(recvJson);
                //                //解压缩
                //                QByteArray cc((char *)m_recvbuf + MSG_HEADER, data_size);
                //                std::string rr = qUncompress(cc).toStdString();
                //                if(rr.size() > 0)
                //                {
                //                    MESSAGE* msg = (MESSAGE*)malloc(sizeof(MESSAGE));
                //                    if (msg == nullptr) {
                //                        LOG_DEBUG << "malloc failed";
                //                    } else {
                //                        memset(msg, 0, sizeof(MESSAGE));
                //                        msg->msg_type = TEXT_RECV;
                //                        msg->ip = ip;
                //                        msg->msg_data = (uchar*)malloc(rr.size());
                //                        if (msg->msg_data == nullptr) {
                //                            free(msg);
                //                            LOG_DEBUG << "malloc msg.data failed";
                //                        } else {
                //                            memset(msg->msg_data, 0, rr.size());
                //                            memcpy_s(msg->msg_data, rr.size(), rr.data(), rr.size());
                //                            msg->msg_len = rr.size();
                //                            recv_queue.push_msg(msg);
                //                        }
                //                    }
                //                }
            }
        }else {
            LOG_DEBUG << "Package error";
        }
    }
}

void MyTcpSocket::stopImmediately()
{
    {
        QMutexLocker lock(&m_lock);
        if(m_isCanRun==true) m_isCanRun=false;
    }
    m_sockThread->quit();
    m_sockThread->wait();
}

void MyTcpSocket::errorDetect(QAbstractSocket::SocketError error)
{
    LOG_DEBUG <<"Sock error" <<QThread::currentThreadId();
    json msg;
    if (error == QAbstractSocket::RemoteHostClosedError) {
        msg["msgType"] = RemoteHostClosedError;
    } else {
        msg["msgType"] = OtherNetError;
    }
    recv_queue.push_msg(&msg);
}


