#ifndef LOGQUEUE_H
#define LOGQUEUE_H

#include <QThread>
#include <QMutex>
#include <QDebug>
#include "netheader.h"
#include "logger.h"

class LogQueue : public QThread
{
    Q_OBJECT
public:
    explicit LogQueue(QObject *parent = nullptr);
    void stopImmediately();
    void pushLog(Log*);
private:
    void run();
    QMutex m_lock;
    bool m_isCanRun;

    MSG_QUEUE<Log> m_logQueue;
    FILE *m_logFile;


signals:

};

#endif // LOGQUEUE_H
