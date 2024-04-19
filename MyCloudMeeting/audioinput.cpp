#include "audioinput.h"

extern MSG_QUEUE<MESSAGE> send_queue;
extern MSG_QUEUE<MESSAGE> recv_queue;

AudioInput::AudioInput(QObject *parent) : QObject(parent)
{
    recvBuf = (char*)malloc(MB * 2);
    QAudioFormat format;//提供音频流的一些信息（参数）
    format.setSampleRate(8000);//采样率
    format.setSampleSize(16);//采样位数
    format.setChannelCount(1);//用来设定声道数目 1 平声道 2 立体声
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::UnSignedInt);

    QAudioDeviceInfo info = QAudioDeviceInfo::defaultInputDevice();
    if (!info.isFormatSupported(format))
    {
        qWarning() << "Default format not supported, trying to use the nearest.";
        format = info.nearestFormat(format);
    }
    audioInput = new QAudioInput(format, this);
    connect(audioInput, SIGNAL(stateChanged(QAudio::State)), this, SLOT(handleStateChange(QAudio::State)));
}

AudioInput::~AudioInput(){
    delete audioInput;
}

void AudioInput::onreadyRead()
{
    static int num = 0, totallen  = 0;
    if (inputDevice == nullptr) return;
    int len = inputDevice->read(recvBuf + totallen, 2 * MB - totallen);
    if (num < 2)
    {
        totallen += len;
        num++;
        return;
    }
    totallen += len;
    MESSAGE* msg = (MESSAGE*)malloc(sizeof(MESSAGE));
    if (msg == nullptr)
    {
        qWarning() << __LINE__ << "malloc fail";
    }
    else
    {
        memset(msg, 0, sizeof(MESSAGE));
        msg->msg_type = AUDIO_SEND;
        QByteArray rr(recvBuf, totallen);
        QByteArray cc = qCompress(rr).toBase64();
        msg->msg_len = cc.size();
        msg->msg_data = (uchar*)malloc(msg->msg_len);
        if (msg->msg_data == nullptr)
        {
            qWarning() << "malloc mesg.data fail";
        }
        else
        {
            memset(msg->msg_data, 0, msg->msg_len);
            memcpy_s(msg->msg_data, msg->msg_len, cc.data(), cc.size());
            send_queue.push_msg(msg);
        }
    }
    totallen = 0;
    num = 0;
}

void AudioInput::handleStateChange(QAudio::State newState)
{
    switch(newState){
    case QAudio::StoppedState:
        if(audioInput->error() != QAudio::NoError){
            stopCollect();
            emit audioInputError(errorString());
        }else{
            qWarning() << "Stop warning";
        }
        break;
    case QAudio::ActiveState:
        qWarning() << "start recording";
        break;
    default:

        break;
    }
}

QString AudioInput::errorString()
{
    if (audioInput->error() == QAudio::OpenError){
        return QString("AudioInput An error occurred opening the audio device").toUtf8();
    }else if (audioInput->error() == QAudio::IOError){
        return QString("AudioInput An error occurred during read/write of audio device").toUtf8();
    }else if (audioInput->error() == QAudio::UnderrunError){
        return QString("AudioInput Audio data is not being fed to the audio device at a fast enough rate").toUtf8();
    }else if (audioInput->error() == QAudio::FatalError){
        return QString("AudioInput A non-recoverable error has occurred, the audio device is not usable at this time.");
    }else{
        return QString("AudioInput No errors have occurred").toUtf8();
    }
}

void AudioInput::setVolum(int v)
{
    audioInput->setVolume(v/100.0);
}

void AudioInput::startCollect()
{
    if(audioInput->state() == QAudio::ActiveState) return;
    WRITE_LOG("start collecting audio");
    inputDevice = audioInput->start();
}

void AudioInput::stopCollect()
{
    if (audioInput->state() == QAudio::StoppedState) return;
    disconnect(this, SLOT(onreadyRead()));
    audioInput->stop();
    WRITE_LOG("stop collecting audio");
    inputDevice = nullptr;
}
