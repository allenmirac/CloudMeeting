#include "audiooutput.h"

#ifndef FRAME_LEN_125MS
#define FRAME_LEN_125MS 1900
#endif
extern MSG_QUEUE<MESSAGE> recv_audioQueue;

AudioOutput::AudioOutput(QObject *parent) : QThread(parent)
{
    QAudioFormat format;
    format.setSampleRate(8000);
    format.setChannelCount(1);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::UnSignedInt);

    QAudioDeviceInfo info = QAudioDeviceInfo::defaultOutputDevice();
    if(!info.isFormatSupported(format)){
        qWarning() << "Raw audio format not supported by backend, cannot play audio.";
        return;
    }
    audioOutput = new QAudioOutput(format, this);
    connect(audioOutput, SIGNAL(stateChanged(QAudio::State)), this, SLOT(handleStateChanged(QAudio::State)));
    outputDevice = nullptr;
}

AudioOutput::~AudioOutput()
{
    if(audioOutput!=nullptr){
        delete audioOutput;
    }
}

void AudioOutput::stopImmediately()
{
    QMutexLocker lock(&m_lock);
    m_isCanRun = false;
}

void AudioOutput::startPlay()
{
    if(audioOutput->state() == QAudio::ActiveState) return;
    LOG_INFO << "Start playing audio";
    audioOutput->start();
}

void AudioOutput::stopPlay()
{
    if(audioOutput->state() == QAudio::StoppedState) return;
    {
        QMutexLocker lock(&deviceLock);
        outputDevice = nullptr;
    }
    audioOutput->stop();
    LOG_INFO << "Stop playing audio";
}

void AudioOutput::handleStateChanged(QAudio::State newState)
{
    switch(newState){
    case QAudio::ActiveState:
        break;
    case QAudio::SuspendedState:
        break;
    case QAudio::StoppedState:
        if(audioOutput->error() != QAudio::NoError){// 音频设备已关闭，检查error是否异常关闭
            audioOutput->stop();
            emit audiooutputerror(errorString());
            LOG_DEBUG << "Out audio error" << audioOutput->error();
        }
        break;
    case QAudio::IdleState:
        break;
    case QAudio::InterruptedState:
        break;
    default:
        break;
    }
}

void AudioOutput::setVolumn(int v)
{
    audioOutput->setVolume(v/100.0);
}

void AudioOutput::clearQueue()
{
    recv_audioQueue.clear();
}

void AudioOutput::run()
{
    m_isCanRun = true;
    QByteArray pcmDataBuffer;

    WRITE_LOG("Start playing audio thread 0x%p", QThread::currentThread());

    for(;;){
        {
            QMutexLocker lock(&m_lock);
            if(m_isCanRun == false){
                stopPlay();
                WRITE_LOG("Stop playing audio thread 0x%p", QThread::currentThread());
                return;
            }
        }
        MESSAGE* msg = recv_audioQueue.pop_msg();
        if(msg == nullptr) continue ;

        {
            QMutexLocker lock(&m_lock);
            if(outputDevice!=nullptr){
                pcmDataBuffer.append((char*)msg->msg_data, msg->msg_len);
                if(pcmDataBuffer.size() >= FRAME_LEN_125MS){
                    qint64 ret = outputDevice->write(pcmDataBuffer.data(), FRAME_LEN_125MS);
                    if(ret < 0){
                        LOG_DEBUG << outputDevice->errorString();
                        return ;
                    }else{
                        emit speaker(QHostAddress(msg->ip).toString());
                        pcmDataBuffer = pcmDataBuffer.right(pcmDataBuffer.size() - ret);
                    }
                }
            }else{
                pcmDataBuffer.clear();
            }
        }
        if(msg->msg_data) free(msg->msg_data);
        if(msg) free(msg);
    }
}

QString AudioOutput::errorString()
{
    if (audioOutput->error() == QAudio::OpenError){
        return QString("AudioInput An error occurred opening the audio device").toUtf8();
    }else if (audioOutput->error() == QAudio::IOError){
        return QString("AudioInput An error occurred during read/write of audio device").toUtf8();
    }else if (audioOutput->error() == QAudio::UnderrunError){
        return QString("AudioInput Audio data is not being fed to the audio device at a fast enough rate").toUtf8();
    }else if (audioOutput->error() == QAudio::FatalError){
        return QString("AudioInput A non-recoverable error has occurred, the audio device is not usable at this time.");
    }else{
        return QString("AudioInput No errors have occurred").toUtf8();
    }
}
