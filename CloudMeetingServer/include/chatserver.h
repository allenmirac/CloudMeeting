#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <iostream>
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>

using namespace muduo;
using namespace muduo::net;

class ChatServer
{
public:
    ChatServer(EventLoop *loop,
               const InetAddress &listenAddr,
               const string &nameArg);
    void initRoom();
    void start();

private:
    void onConnection(const TcpConnectionPtr &conn);

    void onMessage(const TcpConnectionPtr &conn,
                   Buffer *buffer,
                   Timestamp time);

    TcpServer server_; // 服务器主类
    EventLoop *loop_;  // 时间循环的指针
};

#endif // CHATSERVER_H