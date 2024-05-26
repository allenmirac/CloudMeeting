
#include "sendtext.h"

extern MSG_QUEUE<json> send_queue;

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
        m_textQueue_cond.wakeOne();//可以push_back了

        json* send=new json();

        if(text.type == CREATE_MEETING){
            (*send)["msgType"] = CREATE_MEETING;
            (*send)["roomId"] = text.str.toUInt();
            (*send)["ip"] = text.ip;
            LOG_DEBUG << "send_queue push CREATE_MEETING || CLOSE_CAMERA";
            send_queue.push_msg(send);
        } else if(text.type == CLOSE_CAMERA){
            (*send)["msgType"] = CLOSE_CAMERA;
        }else if(text.type == JOIN_MEETING){
            (*send)["msgType"] = JOIN_MEETING;
            (*send)["roomId"] = text.str.toUInt();
            (*send)["ip"] = text.ip;
            LOG_DEBUG << "send_queue push msg JOIN_MEETING, roomId="<<text.str.toUInt();
            //加入发送队列
            send_queue.push_msg(send);
        } else if(text.type == TEXT_SEND){
            (*send)["msgType"] = TEXT_SEND;
            QByteArray data = qCompress(QByteArray::fromStdString(text.str.toStdString())); //压缩
            QString base64CompressedData = QString(data.toBase64());
            (*send)["text"] = base64CompressedData.toStdString();
            (*send)["ip"] = text.ip;
            LOG_DEBUG << "send_queue push msg TEXT_SEND";
            send_queue.push_msg(send);
        } else if( text.type == PARTNER_EXIT) {
            (*send)["msgType"] = PARTNER_EXIT;
            (*send)["roomUserNo"] = text.str.toUInt();
            (*send)["ip"] = text.ip;
            LOG_DEBUG << "send_queue push msg PARTNER_EXIT";
            send_queue.push_msg(send);
        } else if (text.type == AUDIO_SEND) {
            (*send)["msgType"] = AUDIO_SEND;
            LOG_DEBUG << "send_queue push msg AUDIO_SEND";
            send_queue.push_msg(send);
        }
    }
}

void SendText::pushToTextQueue(MSG_TYPE msg_type,  QString str, quint32 ip)
{
    m_textQueue_lock.lock();
    while(m_textQueue.size() > QUEUE_MAXSIZE){
        m_textQueue_cond.wait(&m_textQueue_lock);
    }
    m_textQueue.push_back(TEXTMSG(str, msg_type, ip));
    m_textQueue_lock.unlock();
    m_textQueue_cond.wakeOne();
}

void SendText::stopImmediately()
{
    QMutexLocker lock(&m_lock);
    m_isCanRun = false;
}
