#include "sendtext.h"

extern MSG_QUEUE<MESSAGE> send_queue;

SendText::SendText(QObject *parent) : QThread(parent)
{

}

void SendText::run()
{
    m_isCanRun = true;
    WRITE_LOG("start sending text thread: 0x%p", QThread::currentThreadId());

    for(;;){
        m_textQueue_lock.lock();
        while(m_textQueue.size()==0){
            bool flag = m_textQueue_cond.wait(&m_textQueue_lock, WAITSECONDS*1000);
            if(flag==false){
                QMutexLocker locker(&m_lock);
                if(m_isCanRun == false)
                {
                    m_textQueue_lock.unlock();
                    WRITE_LOG("stop sending text thread: 0x%p", QThread::currentThreadId());
                    return;
                }
            }
        }

        TEXTMSG text = m_textQueue.front();
        m_textQueue.pop_front();
        m_textQueue_lock.unlock();
        m_textQueue_cond.wakeOne();

        MESSAGE *send = (MESSAGE*)malloc(sizeof(MESSAGE));
        if (send == NULL){
            WRITE_LOG("malloc error");
            qDebug() << __FILE__  <<__LINE__ << "malloc fail";
            continue;
        }else{
            memset(send, 0, sizeof(MESSAGE));

            if(text.type == CREATE_MEETING || text.type == CLOSE_CAMERA){
                send->msg_len = 0;
                send->msg_data = NULL;
                send->msg_type = text.type;
                send_queue.push_msg(send);
            }else if(text.type == JOIN_MEETING){
                send->msg_type = JOIN_MEETING;
                send->msg_len = 4; //房间号占4个字节
                send->msg_data = (uchar*)malloc(send->msg_len + 10);
                if (send->msg_data == NULL)
                {
                    WRITE_LOG("malloc error");
                    qDebug() << __FILE__ << __LINE__ << "malloc failed";
                    free(send);
                    continue;
                }else{
                    memset(send->msg_data, 0, send->msg_len + 10);
                    quint32 roomno = text.str.toUInt();
                    memcpy(send->msg_data, &roomno, sizeof(roomno));
                    //加入发送队列
                    send_queue.push_msg(send);
                }
            }else if(text.type == TEXT_SEND){
                send->msg_type = TEXT_SEND;
                QByteArray data = qCompress(QByteArray::fromStdString(text.str.toStdString())); //压缩
                send->msg_len = data.size();
                send->msg_data = (uchar *) malloc(send->msg_len);
                if(send->msg_data == NULL)
                {
                    WRITE_LOG("malloc error");
                    qDebug() << __FILE__ << __LINE__ << "malloc error";
                    free(send);
                    continue;
                }
                else
                {
                    memset(send->msg_data, 0, send->msg_len);
                    memcpy_s(send->msg_data, send->msg_len, data.data(), data.size());
                    send_queue.push_msg(send);
                }
            }
        }
    }
}

void SendText::pushToTextQueue(MSG_TYPE msg_type, QString str)
{
    m_textQueue_lock.lock();
    while(m_textQueue.size() > QUEUE_MAXSIZE){
        m_textQueue_cond.wait(&m_textQueue_lock);
    }
    m_textQueue.push_back(TEXTMSG(str, msg_type));
    m_textQueue_lock.unlock();
}

void SendText::stopImmediately()
{
    QMutexLocker lock(&m_lock);
    m_isCanRun = false;
}
