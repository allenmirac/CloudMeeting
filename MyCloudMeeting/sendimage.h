#ifndef SENDIMAGE_H
#define SENDIMAGE_H

#include <QObject>
#include <QThread>
#include <QQueue>
#include <QMutex>
#include <QImage>
#include <QWaitCondition>
#include <QBuffer>
#include "netheader.h"
#include "logger.h"

class SendImage : public QThread
{
    Q_OBJECT
public:
    explicit SendImage(QObject *parent = nullptr);
    void pushToImgQueue(QImage);
private:
    QQueue<QByteArray> m_imgQueue;
    QMutex m_imgQueue_lock;//队列锁
    QWaitCondition m_imgQueue_cond;
    QMutex m_lock;//是否停止锁
    volatile bool m_isCanRun;

    void run() override;
public slots:
    void ImageCapture(QImage); //捕获到视频帧
    void clearImgQueue(); //线程结束时，清空视频帧队列
    void stopImmediately();
};

#endif // SENDIMAGE_H
