#include "logqueue.h"

LogQueue::LogQueue(QObject *parent) : QThread(parent)
{

}

void LogQueue::stopImmediately()
{
    QMutexLocker lock(&m_lock);
    m_isCanRun=false;
}

void LogQueue::pushLog(Log *log){
    m_logQueue.push_msg(log);
}

void LogQueue::run()
{
    m_isCanRun=true;
    for(;;){
        {
            QMutexLocker lock(&m_lock);
            if(m_isCanRun==false){
                fclose(m_logFile);
                return;
            }
        }
        Log* log=m_logQueue.pop_msg();
        if(log==nullptr || log->data==nullptr){
            continue;
        }
        errno_t error = fopen_s(&m_logFile, "./log.txt", "a");
        if(error!=0){
            LOG_WARN << "打开文件错误！" << error;
            break;
        }

        int hasToWrite = log->len;
        int hasWrite = 0, ret=0;
        while((ret=fwrite(log->data+hasWrite, 1, hasToWrite-hasWrite, m_logFile))<hasToWrite){
            if(ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)){ // non-blocking I/O operations
                ret=0;
            }else{
                LOG_WARN << "Write logFile error!!";
                break;
            }
            hasWrite += ret;
            hasToWrite -= ret;
        }

        if(log->data!=nullptr) free(log->data);
        if(log!=nullptr) free(log);

        fflush(m_logFile);
        fclose(m_logFile);
    }
}

