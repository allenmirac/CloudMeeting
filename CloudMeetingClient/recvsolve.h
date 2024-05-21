#ifndef RECVSOLVE_H
#define RECVSOLVE_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QMetaType>
#include "netheader.h"

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
    void dataRecv(MESSAGE*);
public slots:
    void stopImmediately();
};

#endif // RECVSOLVE_H
