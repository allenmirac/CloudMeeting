#include "audioinput.h"

extern MSG_QUEUE<json> send_queue;
extern MSG_QUEUE<json> recv_queue;

AudioInput::AudioInput(QObject *parent) : QObject(parent)
{
    // recvBuf = (char*)malloc(MB * 2);
    QAudioFormat format;//提供音频流的一些信息（参数）
    format.setSampleRate(8000);//采样率
    format.setSampleSize(16);//采样位数
    format.setChannelCount(1);//用来设定声道数目 1 平声道 2 立体声
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);

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
    //    static int num = 0, totallen  = 0;
    //    if (inputDevice == nullptr) return;
    //    int len = inputDevice->read(recvBuf.data() + totallen, 2 * MB - totallen);
    //    if (num < 2)
    //    {
    //        totallen += len;
    //        num++;
    //        return;
    //    }
    //    totallen += len;
    //    json *msg = new json();
    //    (*msg)["msgType"] = AUDIO_SEND;
    //    //    msg["size"] = cc.size(); // 不知道是否需要加上size字段
    //    QByteArray rr(recvBuf, totallen);
    //    QByteArray cc = qCompress(rr).toBase64();
    //    (*msg)["data"] = cc.toStdString();
    //    send_queue.push_msg(msg);
    //    totallen = 0;
    //    num = 0;
    QByteArray data = inputDevice->read(2 * MB - recvBuf.size());
    recvBuf.append(data);
    numReads = data.size();
    if (++numReads >= 3) {
        json msg;
        msg["msgType"] = AUDIO_SEND;
        QByteArray compressedData = qCompress(recvBuf).toBase64();
        msg["data"] = std::string(compressedData.begin(), compressedData.end());
        send_queue.push_msg(new json(msg));
        LOG_DEBUG << "Audioinput push to send_queue";
        recvBuf.clear();
        numReads = 0;
    }
}

void AudioInput::handleStateChange(QAudio::State newState)
{
    switch(newState){
    case QAudio::StoppedState:
        if(audioInput->error() != QAudio::NoError){
            stopCollect();
            emit audioInputError(errorString());
        }else{
            LOG_WARN << "Stop recording";
        }
        break;
    case QAudio::ActiveState:
        startCollect();
        LOG_WARN << "start recording";
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

void AudioInput::setVolume(int v)
{
    audioInput->setVolume(v/100.0);
}

void AudioInput::startCollect()
{
    if(audioInput->state() == QAudio::ActiveState) return;
    WRITE_LOG("start collecting audio");
    inputDevice = audioInput->start();
    connect(inputDevice, SIGNAL(readyRead()), this, SLOT(onreadyRead()));
}

void AudioInput::stopCollect()
{
    if (audioInput->state() == QAudio::StoppedState) return;
    disconnect(this, SLOT(onreadyRead()));
    audioInput->stop();
    WRITE_LOG("stop collecting audio");
    inputDevice = nullptr;
}
