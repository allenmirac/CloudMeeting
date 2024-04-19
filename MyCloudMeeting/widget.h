#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QVideoFrame>
#include <QTcpSocket>
#include <QCamera>
#include <QCameraViewfinder>
#include <QCameraImageCapture>
#include <QDebug>
#include <QPainter>
#include <QCamera>
#include <QMap>
#include <QStringListModel>
#include <QRegExp>
#include <QRegExpValidator>
#include <QMessageBox>
#include <QScrollBar>
#include <QHostAddress>
#include <QTextCodec>
#include <QDateTime>
#include <QCompleter>
#include <QStringListModel>
#include <QSound>
#include <QString>
#include "audioinput.h"
#include "audiooutput.h"
#include "chatmessage.h"
#include "logqueue.h"
#include "logger.h"
#include "mytcpsocket.h"
#include "myvideosurface.h"
#include "netheader.h"
#include "partner.h"
#include "recvsolve.h"
#include "sendimage.h"
#include "sendtext.h"
#include "screen.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class QCameraImageCapture;
class MyVideoSurface;
class SendImage;
class QListWidgetItem;
class Widget : public QWidget
{
    Q_OBJECT
private:
    static QRect pos;
    quint32 mainip; //主屏幕显示的IP图像
    QCamera *_camera; //摄像头
    QCameraImageCapture *_imagecapture; //截屏
    bool  _createmeet; //是否创建会议
    bool _joinmeet; // 加入会议
    bool _openCamera; //是否开启摄像头

    MyVideoSurface *_myvideosurface;

    QVideoFrame mainshow;

    SendImage *_sendImg;
    QThread *_imgThread;

    RecvSolve *_recvThread;
    SendText * _sendText;
    QThread *_textThread;
    //socket
    MyTcpSocket * _mytcpSocket;
    void paintEvent(QPaintEvent *event);

    QMap<quint32, Partner *> partner; //用于记录房间用户
    Partner* addPartner(quint32);
    void removePartner(quint32);
    void clearPartner(); //退出会议，或者会议结束
    void closeImg(quint32); //根据IP重置图像

    void dealMessage(ChatMessage *messageW, QListWidgetItem *item, QString text, QString time, QString ip ,ChatMessage::User_Type type); //用户发送文本
    void dealMessageTime(QString curMsgTime); //处理时间

    //音频
    AudioInput* _ainput;
    QThread* _ainputThread;
    AudioOutput* _aoutput;

    QStringList iplist;

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private slots:
    void on_createmeetBtn_clicked();
    void on_exitmeetBtn_clicked();

    void on_openvideoBtn_clicked();
    void on_openaudioBtn_clicked();
    void on_connectserverBtn_clicked();
    void cameraError(QCamera::Error);
    void audioError(QString);
//    void mytcperror(QAbstractSocket::SocketError);
    void datasolve(MESSAGE *);
    void recvip(quint32);
    void cameraImageCapture(QVideoFrame frame);
    void on_joinmeetBtn_clicked();

    void on_horizontalSlider_sound_valueChanged(int value);
    void speaks(QString);

    void on_sendMsgBtn_clicked();

    void textSend();

signals:
    void pushImg(QImage);
    void PushText(MSG_TYPE, QString = "");
    void stopAudio();
    void startAudio();
    void volumnChange(int);
private:
    Ui::Widget *ui;
};
#endif // WIDGET_H
