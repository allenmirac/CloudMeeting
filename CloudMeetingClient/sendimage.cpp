#include "sendimage.h"

extern MSG_QUEUE<json> send_queue;

SendImage::SendImage(QObject *parent) : QThread(parent)
{

}

void SendImage::pushToImgQueue(QImage)
{
    QByteArray byte;
    QBuffer buf(&byte);
    buf.open(QIODevice::WriteOnly);
    QByteArray ss = qCompress(byte);
    QByteArray vv = ss.toBase64();
    m_imgQueue_lock.lock();
    while(m_imgQueue.size()>QUEUE_MAXSIZE){
        m_imgQueue_cond.wait(&m_imgQueue_lock);
    }
    m_imgQueue.push_back(vv);
    m_imgQueue_lock.unlock();
    m_imgQueue_cond.wakeOne();
}

void SendImage::run()
{
    WRITE_LOG("start sending picture thread: 0x%p", QThread::currentThreadId());
    m_isCanRun = true;

    for(;;){
        m_imgQueue_lock.lock();
        while(m_imgQueue.size() == 0){
            bool flag = m_imgQueue_cond.wait(&m_imgQueue_lock, WAITSECONDS*1000);

            if(flag == false){
                QMutexLocker locker(&m_lock);
                if (m_isCanRun == false){
                    m_imgQueue_lock.unlock();
                    WRITE_LOG("stop sending picture thread: 0x%p", QThread::currentThreadId());
                    return;
                }
            }
        }
        QByteArray img = m_imgQueue.front();
        m_imgQueue.pop_front();
        m_imgQueue_lock.unlock();//解锁
        m_imgQueue_cond.wakeOne(); //唤醒添加线程

        json *imgSend = new json;
        (*imgSend)["msgType"] = IMG_SEND;
        QByteArray data = qCompress(QByteArray::fromStdString(img.toStdString())); //压缩
        QString base64CompressedData = QString(data.toBase64());
        LOG_DEBUG << "img size :" << img.size();
        (*imgSend)["data"] = base64CompressedData.toStdString();
        send_queue.push_msg(imgSend);
    }

}

void SendImage::ImageCapture(QImage img)
{
    pushToImgQueue(img);
}

void SendImage::clearImgQueue()
{
    LOG_DEBUG << "清空视频队列";
    QMutexLocker lock(&m_lock);
    m_imgQueue.clear();
}

void SendImage::stopImmediately()
{
    QMutexLocker lock(&m_lock);
    m_isCanRun = false;
}
