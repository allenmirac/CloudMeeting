#ifndef AUDIOOUTPUT_H
#define AUDIOOUTPUT_H

#include <QObject>
#include <QThread>
#include <QAudioOutput>
#include <QMutex>
#include <QDebug>
#include <QHostAddress>
#include "netheader.h"
#include "logger.h"

class AudioOutput : public QThread
{
    Q_OBJECT
public:
    explicit AudioOutput(QObject *parent = nullptr);
    ~AudioOutput();
    void stopImmediately();
    void startPlay();
    void stopPlay();

signals:
    void audiooutputerror(QString);
    void speaker(QString);
private slots:
    void handleStateChanged(QAudio::State);
    void setVolume(int);
    void clearQueue();

private:
    QAudioOutput *audioOutput;
    QIODevice *outputDevice;
    QMutex deviceLock;
    volatile bool m_isCanRun;
    QMutex m_lock;
    void run();
    QString errorString();
};

#endif // AUDIOOUTPUT_H
