#ifndef SENDTEXT_H
#define SENDTEXT_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QQueue>
#include <QWaitCondition>
#include "netheader.h"
#include "logger.h"
#include "json.h"
using json = nlohmann::json;

struct TEXTMSG{
    QString str;
    MSG_TYPE type;
    quint32 ip;

    TEXTMSG(QString s, MSG_TYPE e, quint32 ip)
    {
        str = s;
        type = e;
        this->ip = ip;
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
    void pushToTextQueue(MSG_TYPE,  QString str="", quint32 ip=0);
    void stopImmediately();
};

#endif // SENDTEXT_H
