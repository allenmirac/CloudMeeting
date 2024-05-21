#include "recvsolve.h"

extern MSG_QUEUE<MESSAGE> recv_queue;

RecvSolve::RecvSolve(QObject *parent) : QThread(parent)
{
    qRegisterMetaType<MESSAGE *>();
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
        MESSAGE * msg = recv_queue.pop_msg();
        if(msg == NULL) continue;
        /*else free(msg);
        qDebug() << "取出队列:" << msg->msg_type;*/
        emit dataRecv(msg);
    }
}

void RecvSolve::stopImmediately()
{
    QMutexLocker lock(&m_lock);
    m_isCanRun = false;
}


