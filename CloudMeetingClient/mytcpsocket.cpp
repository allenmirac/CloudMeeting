#include "mytcpsocket.h"

extern MSG_QUEUE<MESSAGE> send_queue;
extern MSG_QUEUE<MESSAGE> recv_queue;
extern MSG_QUEUE<MESSAGE> recv_audioQueue;

MyTcpSocket::MyTcpSocket(QObject* parent) : QThread(parent)
{
    qRegisterMetaType<QAbstractSocket::SocketError>();
    m_socktcp=nullptr;

    m_sockThread=new QThread();
    this->moveToThread(m_sockThread);
    connect(m_sockThread, SIGNAL(finished()), this, SLOT(closeSocket()));

    m_sendbuf = (uchar*)malloc(4 * MB);
    m_recvbuf = (uchar*)malloc(4*MB);
    m_hasReceive = 0;
}

MyTcpSocket::~MyTcpSocket()
{
    if(m_sendbuf == nullptr){
        free(m_sendbuf);
    }
    if(m_recvbuf == nullptr){
        free(m_recvbuf);
    }
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
    for(;;){
        {
            QMutexLocker lock(&m_lock);
            if(m_isCanRun==false) return ;
        }
        MESSAGE *send = send_queue.pop_msg();
        if(send==nullptr) continue;
        QMetaObject::invokeMethod(this, "sendData", Q_ARG(MESSAGE*, send));
    }
}

qint64 MyTcpSocket::readn(char *buf, quint64 maxSize, int n)
{
    quint64 hasToRead = n;
    quint64 hasRead = 0;
    do{
        quint64 ret = m_socktcp->read(buf+hasRead, hasToRead);
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
    m_socktcp->connectToHost(ip, port.toUShort(), flag);
    // 最后一个参数保证连接到槽不会重复连接
    connect(m_socktcp, SIGNAL(readyRead()), this, SLOT(recvFromSocket()), Qt::UniqueConnection);\
    if(m_socktcp->waitForConnected(5000)){
        return true;
    }
    m_socktcp->close();
    return false;
}

void MyTcpSocket::sendData(MESSAGE *send)
{
    if(m_socktcp->state() == QAbstractSocket::UnconnectedState){
        emit sendTextOver();
        if (send->msg_data) free(send->msg_data);
        if (send) free(send);
        return;
    }
    quint64 bytesToWrite = 0;

    // write to dets in big-endian byte order.
    //构造消息头
    m_sendbuf[bytesToWrite++] = '$';

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
    m_sendbuf[bytesToWrite++] = '#'; //结尾字符

    qint64 hastowrite = bytesToWrite;
    qint64 ret = 0, haswrite = 0;
    while ((ret = m_socktcp->write((char*)m_sendbuf + haswrite, hastowrite - haswrite)) < hastowrite)
    {
        if (ret == -1 && m_socktcp->error() == QAbstractSocket::TemporaryError) {
            ret = 0;
        } else if (ret == -1) {
            qDebug() << "network error";
            break;
        }
        haswrite += ret;
        hastowrite -= ret;
    }

    m_socktcp->waitForBytesWritten();
    if(send->msg_type == TEXT_SEND){
        emit sendTextOver();
    }
    if(send->msg_data) free(send->msg_data);
    if(send) free(send);
}

void MyTcpSocket::closeSocket()
{
    if(m_socktcp && m_socktcp->isOpen()){
        m_socktcp->close();
    }
}

void MyTcpSocket::recvFromSocket()
{
    qint64 availbytes = m_socktcp->bytesAvailable();
    if (availbytes <=0 ) {
        return;
    }
    qint64 ret = m_socktcp->read((char *) m_recvbuf + m_hasReceive, availbytes);
    if (ret <= 0) {
        qDebug() << "error or no more data";
        return;
    }
    m_hasReceive += ret;

    //数据包不够
    if (m_hasReceive < MSG_HEADER) {
        return;
    }else{
        quint32 data_size;
        qFromBigEndian<quint32>(m_recvbuf+7, 4, &data_size);//将数据大小读入data_szie
        if((quint64)data_size+1+MSG_HEADER <= m_hasReceive){
            if (m_recvbuf[0] == '$' && m_recvbuf[MSG_HEADER + data_size] == '#'){
                MSG_TYPE msgtype;
                uint16_t type;
                qFromBigEndian<uint16_t>(m_recvbuf + 1, 2, &type);
                msgtype=(MSG_TYPE)type;
                if (msgtype == CREATE_MEETING_RESPONSE || msgtype == JOIN_MEETING_RESPONSE
                        || msgtype == PARTNER_JOIN2){
                    if (msgtype == CREATE_MEETING_RESPONSE) {
                        qint32 roomNo;
                        qFromBigEndian<qint32>(m_recvbuf + MSG_HEADER, 4, &roomNo);
                        MESSAGE *msg = (MESSAGE*)malloc(sizeof(MESSAGE));
                        if(msg == nullptr){
                            LOG_DEBUG << " CREATE_MEETING_RESPONSE malloc MESG failed";
                        }else{
                            memset(msg, 0, sizeof(MESSAGE));

                            msg->msg_type=msgtype;
                            msg->msg_data = (uchar*)malloc((quint64)data_size);
                            if(msg->msg_data == nullptr){
                                free(msg->msg_data);
                                LOG_DEBUG << "CREATE_MEETING_RESPONSE malloc MESG.data failed";
                            }else{
                                memset(msg->msg_data, 0, (quint64)data_size);
                                memcpy(msg->msg_data, &roomNo, data_size);
                                msg->msg_len=data_size;
                                recv_queue.push_msg(msg);
                            }
                        }
                    }else if(msgtype == JOIN_MEETING_RESPONSE) {
                        qint32 c;
                        memcpy(&c, m_recvbuf + MSG_HEADER, data_size);

                        MESSAGE* msg = (MESSAGE*)malloc(sizeof(MESSAGE));
                        if (msg == nullptr)
                        {
                            LOG_DEBUG << "JOIN_MEETING_RESPONSE malloc MESG failed";
                        } else {
                            memset(msg, 0, sizeof(MESSAGE));
                            msg->msg_type = msgtype;
                            msg->msg_data = (uchar*)malloc(data_size);
                            if (msg->msg_data == NULL) {
                                free(msg);
                                LOG_DEBUG << "JOIN_MEETING_RESPONSE malloc MESG.data failed";
                            } else {
                                memset(msg->msg_data, 0, data_size);
                                memcpy(msg->msg_data, &c, data_size);

                                msg->msg_len = data_size;
                                recv_queue.push_msg(msg);
                            }
                        }
                    }else if(msgtype == PARTNER_JOIN2){
                        MESSAGE* msg = (MESSAGE*)malloc(sizeof(MESSAGE));
                        if (msg == nullptr) {
                            LOG_DEBUG << "PARTNER_JOIN2 malloc MESG error";
                        }else {
                            memset(msg, 0, sizeof(MESSAGE));
                            msg->msg_type = msgtype;
                            msg->msg_len = data_size;
                            msg->msg_data = (uchar*)malloc(data_size);
                            if (msg->msg_data == nullptr) {
                                free(msg);
                                LOG_DEBUG << "PARTNER_JOIN2 malloc MESG.data error";
                            } else {
                                memset(msg->msg_data, 0, data_size);
                                uint32_t ip;
                                int pos = 0;
                                for (int i = 0; i < data_size / sizeof(uint32_t); i++) {\
                                    // 一次读四个字节 代表房间成员ip地址
                                    qFromBigEndian<uint32_t>(m_recvbuf + MSG_HEADER + pos, sizeof(uint32_t), &ip);
                                    memcpy_s(msg->msg_data + pos, data_size - pos, &ip, sizeof(uint32_t));
                                    pos += sizeof(uint32_t);
                                }
                                recv_queue.push_msg(msg);
                            }
                        }
                    }
                }else if (msgtype == IMG_RECV || msgtype == PARTNER_JOIN
                          || msgtype == PARTNER_EXIT || msgtype == AUDIO_RECV
                          || msgtype == CLOSE_CAMERA || msgtype == TEXT_RECV) {
                    quint32 ip;
                    qFromBigEndian<quint32>(m_recvbuf + 3, 4, &ip);
                    if (msgtype == IMG_RECV) {
                        QByteArray cc((char *) m_recvbuf + MSG_HEADER, data_size);
                        QByteArray rc = QByteArray::fromBase64(cc);
                        QByteArray rdc = qUncompress(rc);//解压缩
                        //将消息加入到接收队列
                        if (rdc.size() > 0)
                        {
                            MESSAGE* msg = (MESSAGE*)malloc(sizeof(MESSAGE));
                            if (msg == nullptr) {
                                LOG_DEBUG << " malloc failed";
                            } else {
                                memset(msg, 0, sizeof(MESSAGE));
                                msg->msg_type = msgtype;
                                msg->msg_data = (uchar*)malloc(rdc.size()); // 10 = format + width + width
                                if (msg->msg_data == NULL) {
                                    free(msg);
                                    LOG_DEBUG << " malloc failed";
                                } else {
                                    memset(msg->msg_data, 0, rdc.size());
                                    memcpy_s(msg->msg_data, rdc.size(), rdc.data(), rdc.size());
                                    msg->msg_len = rdc.size();
                                    msg->ip = ip;
                                    recv_queue.push_msg(msg);
                                }
                            }
                        }
                    } else if (msgtype == PARTNER_JOIN || msgtype == PARTNER_EXIT || msgtype == CLOSE_CAMERA) {
                        MESSAGE* msg = (MESSAGE*)malloc(sizeof(MESSAGE));
                        if (msg == nullptr) {
                            LOG_DEBUG << " malloc failed";
                        } else {
                            memset(msg, 0, sizeof(MESSAGE));
                            msg->msg_type = msgtype;
                            msg->ip = ip;
                            recv_queue.push_msg(msg);
                        }
                    } else if (msgtype == AUDIO_RECV) {
                        QByteArray cc((char*)m_recvbuf + MSG_HEADER, data_size);
                        QByteArray rc = QByteArray::fromBase64(cc);
                        QByteArray rdc = qUncompress(rc);

                        if (rdc.size() > 0) {
                            MESSAGE* msg = (MESSAGE*)malloc(sizeof(MESSAGE));
                            if (msg == nullptr) {
                                LOG_DEBUG << "malloc failed";
                            } else {
                                memset(msg, 0, sizeof(MESSAGE));
                                msg->msg_type = AUDIO_RECV;
                                msg->ip = ip;

                                msg->msg_data = (uchar*)malloc(rdc.size());
                                if (msg->msg_data == nullptr)
                                {
                                    free(msg);
                                    LOG_DEBUG << "malloc msg.data failed";
                                }
                                else
                                {
                                    memset(msg->msg_data, 0, rdc.size());
                                    memcpy_s(msg->msg_data, rdc.size(), rdc.data(), rdc.size());
                                    msg->msg_len = rdc.size();
                                    msg->ip = ip;
                                    recv_audioQueue.push_msg(msg);
                                }
                            }
                        }
                    } else if(msgtype == TEXT_RECV) {
                        //解压缩
                        QByteArray cc((char *)m_recvbuf + MSG_HEADER, data_size);
                        std::string rr = qUncompress(cc).toStdString();
                        if(rr.size() > 0)
                        {
                            MESSAGE* msg = (MESSAGE*)malloc(sizeof(MESSAGE));
                            if (msg == nullptr) {
                                LOG_DEBUG << "malloc failed";
                            } else {
                                memset(msg, 0, sizeof(MESSAGE));
                                msg->msg_type = TEXT_RECV;
                                msg->ip = ip;
                                msg->msg_data = (uchar*)malloc(rr.size());
                                if (msg->msg_data == nullptr) {
                                    free(msg);
                                    LOG_DEBUG << "malloc msg.data failed";
                                } else {
                                    memset(msg->msg_data, 0, rr.size());
                                    memcpy_s(msg->msg_data, rr.size(), rr.data(), rr.size());
                                    msg->msg_len = rr.size();
                                    recv_queue.push_msg(msg);
                                }
                            }
                        }
                    }
                }else {
                    LOG_DEBUG << "Package error";
                }
                memmove_s(m_recvbuf, 4 * MB, m_recvbuf + MSG_HEADER + data_size + 1, m_hasReceive - ((quint64)data_size + 1 + MSG_HEADER));
                m_hasReceive -= ((quint64)data_size + 1 + MSG_HEADER);
            }
        }else {
            return ;
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
    MESSAGE *msg = (MESSAGE*)malloc(sizeof(MESSAGE));
    if(msg == nullptr){
        LOG_DEBUG << "errdect malloc error";
    } else {
        memset(msg, 0, sizeof(MESSAGE));
        if (error == QAbstractSocket::RemoteHostClosedError) {
            msg->msg_type = RemoteHostClosedError;
        } else {
            msg->msg_type = OtherNetError;
        }
        recv_queue.push_msg(msg);
    }
}


