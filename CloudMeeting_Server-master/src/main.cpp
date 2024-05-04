#include <iostream>
#include <signal.h>
#include "chatserver.h"
#include "chatservice.h"

using namespace std;

// 服务器异常，业务重置
void statusResetHandler(int)
{

    exit(0);
}

int main()
{
    signal(SIGINT, statusResetHandler);

    EventLoop loop;
    InetAddress addr("0.0.0.0", 2222);
    ChatServer server(&loop, addr, "ChatServer");

    server.initRoom(); // 创建五个房间
    server.start();
    loop.loop();

    return 0;
}