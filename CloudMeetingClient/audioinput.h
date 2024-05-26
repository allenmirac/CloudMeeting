#ifndef AUDIOINPUT_H
#define AUDIOINPUT_H

#include <QObject>
#include <QIODevice>
#include <QAudioInput>
#include <QAudioFormat>
#include <QDebug>
#include <QByteArray>
#include <QThread>
#include "netheader.h"
#include "logger.h"

class AudioInput : public QObject
{
    Q_OBJECT
public:
    explicit AudioInput(QObject *parent = nullptr);
    ~AudioInput();

signals:
    void audioInputError(QString);
private slots:
    void onreadyRead();
    void handleStateChange(QAudio::State);
    QString errorString();

public slots:
    void startCollect();
    void stopCollect();
    void setVolume(int);
private:
    QAudioInput *audioInput=nullptr;
    QIODevice *inputDevice=nullptr;
    QByteArray recvBuf;
    int totalLength = 0;
    int numReads = 0;
};

#endif // AUDIOINPUT_H
