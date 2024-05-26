#include "recvsolve.h"

extern MSG_QUEUE<json> recv_queue;

RecvSolve::RecvSolve(QObject *parent) : QThread(parent)
{
    qRegisterMetaType<json *>();
    m_isCanRun = true;
}

void RecvSolve::run()
{
    WRITE_LOG("start solving data thread: 0x%p", QThread::currentThreadId());
    for(;;) {
        {
            QMutexLocker locker(&m_lock);
            if (m_isCanRun == false) {
                WRITE_LOG("stop solving data thread: 0x%p", QThread::currentThreadId());
                return;
            }
        }
        json* msg = recv_queue.pop_msg();
        if(msg == NULL) continue;
        // else free(msg);
        // LOG_INFO << "取出的消息长度: " << msg->msg_len;
        emit dataRecv(msg);
    }
}

void RecvSolve::stopImmediately()
{
    QMutexLocker lock(&m_lock);
    m_isCanRun = false;
}


