#ifndef AUDIOINPUT_H
#define AUDIOINPUT_H

#include <QObject>
#include <QIODevice>
#include <QAudioInput>
#include <QAudioFormat>
#include <QDebug>
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
    void setVolum(int);
public slots:
    void startCollect();
    void stopCollect();
private:
    QAudioInput *audioInput;
    QIODevice *inputDevice;
    char *recvBuf;
};

#endif // AUDIOINPUT_H
