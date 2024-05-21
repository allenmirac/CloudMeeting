#include "netheader.h"
#include "logqueue.h"

MSG_QUEUE<MESSAGE> send_queue;
MSG_QUEUE<MESSAGE> recv_queue;
MSG_QUEUE<MESSAGE> recv_audioQueue;

LogQueue* logqueue = nullptr;

void log_print(const char* filename, const char*funcname, int line, const char* fmt, ...){
    Log* log = (Log*)malloc(KB * 1);
    if(log==nullptr){
        qDebug() << "malloc log failed!!";
        return ;
    }else{
        memset(log, 0, sizeof(Log));
        log->data = (char*)malloc(KB * 1);
        if(log->data==nullptr){
            qDebug() << "malloc log data failed!!";
            return;
        }
        memset(log->data, 0, 1*KB);
        time_t t = time(NULL);
        int pos = 0;
        int m = strftime(log->data+pos, KB-2-pos, "%Y-%m-%d %H:%M:%S", localtime(&t));
        pos += m;

        m = snprintf(log->data+pos, KB-2-pos, "%s:%s::%d>>>", filename, funcname, line);
        pos += m;

        va_list ap;
        va_start(ap, fmt);
        m=_vsnprintf(log->data+pos, KB-2-pos, fmt, ap);
        pos+=m;
        va_end(ap);
        strcat_s(log->data+pos, KB-pos, "\n");
        log->len = strlen(log->data);
        logqueue->pushLog(log);
    }
}
