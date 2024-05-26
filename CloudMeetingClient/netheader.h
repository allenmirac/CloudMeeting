#ifndef NETHEADER_H
#define NETHEADER_H
#include <QMetaType>
#include <QMutex>
#include <QQueue>
#include <QImage>
#include <QWaitCondition>
#include "json.h"
using json = nlohmann::json;
#define QUEUE_MAXSIZE 1500
#ifndef MB
#define MB 1024*1024
#endif

#ifndef KB
#define KB 1024
#endif

#ifndef WAITSECONDS
#define WAITSECONDS 2
#endif

#ifndef OPENVIDEO
#define OPENVIDEO "打开视频"
#endif

#ifndef CLOSEVIDEO
#define CLOSEVIDEO "关闭视频"
#endif

#ifndef OPENAUDIO
#define OPENAUDIO "打开音频"
#endif

#ifndef CLOSEAUDIO
#define CLOSEAUDIO "关闭音频"
#endif

#ifndef MSG_HEADER
#define MSG_HEADER 11 // 1+2+4+4 头+消息类型+ip+消息长度
#endif

enum MSG_TYPE
{
    IMG_SEND = 0,
    IMG_RECV,
    AUDIO_SEND,
    AUDIO_RECV,
    TEXT_SEND,
    TEXT_RECV,
    CREATE_MEETING,
    EXIT_MEETING,
    JOIN_MEETING,
    CLOSE_CAMERA,

    CREATE_MEETING_RESPONSE = 20,
    PARTNER_EXIT = 21,
    PARTNER_JOIN = 22,
    JOIN_MEETING_RESPONSE = 23,
    PARTNER_JOIN2 = 24,
    BROAD_MESSAGE = 25,
    BROAD_ADDUSER_MESSAGE = 26,
    RemoteHostClosedError = 40,
    OtherNetError = 41
};

struct MESSAGE
{
    MSG_TYPE msg_type;
    uchar* msg_data;
    ulong msg_len; // 不包括头部
    quint32 ip;
};

Q_DECLARE_METATYPE(json*)
Q_DECLARE_METATYPE(MSG_TYPE)

template<class T>
class MSG_QUEUE
{
public:
    MSG_QUEUE(){
//        send_queue = QQueue<T*>(1);
    }
    void push_msg(T* msg){
        send_lock.lock();
        int queueSize = send_queue.size();
        while(queueSize > QUEUE_MAXSIZE){
            send_cond.wait(&send_lock);
        }
        send_queue.push_back(msg);
        send_lock.unlock();
        send_cond.wakeOne();
    }
    T* pop_msg(){
        send_lock.lock();
        while(send_queue.size()==0){
            bool flag=send_cond.wait(&send_lock, WAITSECONDS * 1000);
            if(!flag){
                send_lock.unlock();
                return nullptr;
            }
        }
        T* tempMsg = send_queue.front();
        send_queue.pop_front();
        send_lock.unlock();
        send_cond.wakeOne();
        return tempMsg;
    }
    void clear(){
        send_lock.lock();
        send_queue.clear();
        send_lock.unlock();
    }
private:
    static QMutex send_lock;
    static QWaitCondition send_cond;
    QQueue<T*> send_queue;
};

template<class T>
QMutex MSG_QUEUE<T>::send_lock;
template<class T>
QWaitCondition MSG_QUEUE<T>::send_cond;

struct Log{
    char* data;
    uint len;
};

void log_print(const char*, const char*, int, const char*, ...);
#define WRITE_LOG(LOGTEXT, ...) do{ \
    log_print(__FILE__, __FUNCTION__, __LINE__, LOGTEXT, ##__VA_ARGS__); \
}while(0);

#endif // NETHEADER_H
