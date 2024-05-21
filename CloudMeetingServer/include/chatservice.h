#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <muduo/net/TcpConnection.h>
#include <mutex>
#include <muduo/base/Condition.h>
#include <unordered_map>
#include <functional>
#include <muduo/base/Logging.h>
#include <vector>
#include "room.h"
#include "public.h"

using namespace muduo;
using namespace muduo::net;

#include "json.hpp"
using json = nlohmann::json;

// 将其当成函数指针来使用
using MsgHandler = std::function<void(const TcpConnectionPtr &conn, json &js, Timestamp)>;


class ChatService{
public:
    static ChatService *instance();
    MsgHandler getHandler(int msgid);
    void clientQuitEcption(const TcpConnectionPtr &conn);
    void setRoom(std::vector<Room> &vecRoom);
    void broadMsg(const TcpConnectionPtr &conn, json &js, Timestamp time);
    void joinMeeting(const TcpConnectionPtr &conn, json &js, Timestamp time);
    void createMeeting(const TcpConnectionPtr &conn, json &js, Timestamp time);
    void quitMeeting(const TcpConnectionPtr &conn, json &js, Timestamp time);
    void audioSend(const TcpConnectionPtr &conn, json &js, Timestamp time);
    void textSend(const TcpConnectionPtr &conn, json &js, Timestamp time);
private:
    ChatService();
    ~ChatService();
    ChatService(const ChatService &) = delete;
    ChatService &operator=(const ChatService &) = delete;
    
private:
    // 存储消息id和其对应的业务处理方法
    std::unordered_map<int, MsgHandler> msgHandlerMap_;

    // 存储连接信息 ip conn
    std::unordered_map<uint32_t, TcpConnectionPtr> userConnMap_;

    // 连接锁
    std::mutex mutex_;
    std::vector<Room> vecRoom_;
};

#endif // CHATSERVICE_H