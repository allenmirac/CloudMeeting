#ifndef RECVSOLVE_H
#define RECVSOLVE_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QMetaType>
#include "netheader.h"
#include "logger.h"

#include "json.h"
using json = nlohmann::json;

class RecvSolve : public QThread
{
    Q_OBJECT
public:
    explicit RecvSolve(QObject *parent = nullptr);
    void run() override;
private:
    QMutex m_lock;
    bool m_isCanRun;
signals:
    void dataRecv(json*);
public slots:
    void stopImmediately();
};

#endif // RECVSOLVE_H
