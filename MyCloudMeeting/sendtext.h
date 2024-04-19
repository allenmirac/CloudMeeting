#ifndef SENDTEXT_H
#define SENDTEXT_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QQueue>
#include <QWaitCondition>
#include "netheader.h"
#include "logger.h"

struct TEXTMSG{
    QString str;
    MSG_TYPE type;

    TEXTMSG(QString s, MSG_TYPE e)
    {
        str = s;
        type = e;
    }
};

class SendText : public QThread
{
    Q_OBJECT
public:
    explicit SendText(QObject *parent = nullptr);

private:
    QQueue<TEXTMSG> m_textQueue;
    QMutex m_textQueue_lock;
    QWaitCondition m_textQueue_cond;
    QMutex m_lock;
    volatile bool m_isCanRun;//确保对变量的操作是线程安全的
    void run() override;
public slots:
    void pushToTextQueue(MSG_TYPE, QString str="");
    void stopImmediately();
};

#endif // SENDTEXT_H
