#include "chatservice.h"

ChatService *ChatService::instance()
{
    static ChatService service;
    return &service;
}

ChatService::ChatService()
{
    // 绑定事件
    msgHandlerMap_.insert({JOIN_MEETING, std::bind(&ChatService::joinMeeting, this, _1, _2, _3)});
    msgHandlerMap_.insert({CREATE_MEETING, std::bind(&ChatService::createMeeting, this, _1, _2, _3)});
    msgHandlerMap_.insert({AUDIO_SEND, std::bind(&ChatService::audioSend, this, _1, _2, _3)});
    msgHandlerMap_.insert({PARTNER_EXIT, std::bind(&ChatService::quitMeeting, this, _1, _2, _3)});
    msgHandlerMap_.insert({TEXT_SEND, std::bind(&ChatService::textSend, this, _1, _2, _3)});
}

ChatService::~ChatService()
{
    msgHandlerMap_.clear();
    userConnMap_.clear();
}

MsgHandler ChatService::getHandler(int msgid)
{
    auto it = msgHandlerMap_.find(msgid);
    if (it == msgHandlerMap_.end())
    {
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp)
        {
            LOG_ERROR << "Msgid is " << msgid << " can not find handler!";
        };
    }
    else
    {
        return msgHandlerMap_[msgid];
    }
}

void ChatService::setRoom(std::vector<Room> &vecRoom)
{
    vecRoom_ = vecRoom;
}

void ChatService::broadMsg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int roomUserNo = js["roomUserNo"];
    if (roomUserNo <= 1)
    {
        return;
    }
    LOG_DEBUG << "需要广播消息";
    std::vector<uint32_t> vecIp;
    uint32_t ip = js["ip"];
    uint roomId = js["roomId"];
    Room room(0);
    for (size_t i = 0; i < vecRoom_.size(); i++)
    {
        if (vecRoom_[i].getRoomId() == roomId)
        {
            room = vecRoom_[i];
            break;
        }
    }
    vecIp = room.getRoomUserIp();
    js["roomUserIp"] = vecIp;
    // for(size_t i=0; i<vecIp.size(); i++){
    //     // if(ip!=vecIp[i]){
    //         TcpConnectionPtr conn = userConnMap_.find(vecIp[i])->second;
    //         conn->send(js.dump());
    //     // }
    // }

    // multimap test
    auto nums = userConnMap_.count(ip);
    auto it = userConnMap_.find(ip);
    while (nums--)
    {
        TcpConnectionPtr conn = it->second;
        it++;
        conn->send(js.dump());
    }
}

void ChatService::joinMeeting(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    uint roomId;
    uint32_t userIp;
    User *user;
    int msgType = js["msgType"];
    LOG_DEBUG << "join Meeting";
    if (msgType == JOIN_MEETING)
    {
        roomId = js["roomId"];
        userIp = js["ip"];

        bool flag = false;
        json response; // 还不清楚要回复多少消息回去，下次再写
        size_t roomUserNo = 0;
        size_t i;
        for (i = 0; i < vecRoom_.size(); i++)
        {
            if (vecRoom_[i].getRoomId() == roomId)
            {
                flag = true;
                roomUserNo = vecRoom_[i].getRoomUserNo();
                break;
            }
        }
        if (flag)
        {
            if (roomUserNo < 1024)
            {
                user = new User(userIp, roomId);
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    vecRoom_[i].addUser(user);
                    userConnMap_.insert({userIp, conn});
                }
                json broadAddUserMsg = vecRoom_[i].addUserMessage(user);
                broadMsg(conn, broadAddUserMsg, time);
                response["msgType"] = JOIN_MEETING_RESPONSE;
                response["roomUserNo"] = vecRoom_[i].getRoomUserNo();
                // broadMsg()
                LOG_INFO << "加入会议成功";
            }
            else
            {
                response["msgType"] = JOIN_MEETING_RESPONSE;
                response["roomUserNo"] = 0;
                LOG_ERROR << "会议人数已满";
            }
        }
        else
        {
            response["msgType"] = JOIN_MEETING_RESPONSE;
            response["roomUserNo"] = -1;
            LOG_ERROR << "房间号不存在";
        }
        conn->send(response.dump());
    }
    else
    {
        LOG_ERROR << "msg type error";
    }
}

void ChatService::createMeeting(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    uint roomId;
    uint32_t userIp;
    User *user;
    int msgType = js["msgType"];
    if (msgType == CREATE_MEETING)
    {
        roomId = js["roomId"];
        userIp = js["ip"];

        bool flag = true;
        json response;
        size_t i;
        for (i = 0; i < vecRoom_.size(); i++)
        {
            if (vecRoom_[i].getRoomId() == roomId)
            {
                flag = false;
                break;
            }
        }
        if (flag)
        {
            user = new User(userIp, roomId);
            Room room(roomId);
            {
                std::lock_guard<std::mutex> lock(mutex_);
                vecRoom_.push_back(room);
                vecRoom_[vecRoom_.size() - 1].addUser(user);
                userConnMap_.insert({userIp, conn});
            }
            response["msgType"] = CREATE_MEETING_RESPONSE;
            response["roomUserNo"] = vecRoom_[i].getRoomUserNo();
            response["roomId"] = roomId;
            LOG_INFO << "创建房间" << roomId << "成功";
        }
        else
        {
            response["msgType"] = CREATE_MEETING_RESPONSE;
            response["roomUserNo"] = -1;
            LOG_INFO << "房间号已存在";
        }
        // LOG_DEBUG << "创建房间消息回复成功";
        conn->send(response.dump());
    }
    else
    {
        LOG_ERROR << "msg type error";
    }
}

void ChatService::quitMeeting(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    // int roomUserNo = js.at("roomUserNo");
    uint32_t ip = js.at("ip");
    MSG_TYPE msgType = js.at("msgType");
    if (msgType == PARTNER_EXIT)
    {
        if (ip != 0)
        {
            for (size_t i = 0; i < vecRoom_.size(); i++)
            {
                User *user = vecRoom_[i].getUser(ip);
                if (user != nullptr)
                {
                    vecRoom_[i].removeUser(user);
                    conn->send(js.dump());
                    LOG_ERROR << "ChatService::clientQuitEcption, 用户下线";
                    break;
                }
            }
        }
        else
        {
            LOG_ERROR << "ChatService::clientQuitEcption, 无此用户";
        }
    }
    else
    {
        LOG_ERROR << "msg type error";
    }
}

void ChatService::audioSend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    MSG_TYPE msgType = js.at("msgType");
    std::string data = js.at("data");
    if (msgType == AUDIO_SEND)
    {
        json response;
        uint32_t ip = 0;
        std::vector<uint32_t> vecIp;
        for (auto it = userConnMap_.begin(); it != userConnMap_.end(); it++)
        {
            if (it->second == conn)
            {
                ip = it->first;
                break;
            }
        }
        for (size_t i = 0; i < vecRoom_.size(); i++)
        {
            User *user = vecRoom_[i].getUser(ip);
            if (user != nullptr)
            {
                vecIp = vecRoom_[i].getRoomUserIp();
                break;
            }
        }
        response["msgType"] = AUDIO_RECV;
        response["data"] = data;
        response["ip"] = ip;
        // // real
        // for(size_t i=0; i<vecIp.size(); i++)
        // {
        //     if(ip!=vecIp[i]){
        //         TcpConnectionPtr conn = userConnMap_[vecIp[i]];
        //         conn->send(response.dump());
        //     }
        // }

        // multimap test
        auto nums = userConnMap_.count(ip);
        auto it = userConnMap_.find(ip);
        while (nums--)
        {
            TcpConnectionPtr sendConn = it->second;
            if(conn != sendConn){
                conn->send(js.dump());
            }
            it++;
        }
    }
}

void ChatService::textSend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    js.at("msgType") = TEXT_RECV;
    broadMsg(conn, js, time);
}

void ChatService::clientQuitEcption(const TcpConnectionPtr &conn)
{
    uint32_t ip = 0;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto it = userConnMap_.begin(); it != userConnMap_.end(); it++)
        {
            if (it->second == conn)
            {
                ip = it->first;
                userConnMap_.erase(it);
                break;
            }
        }
    }
    if (ip != 0)
    {
        for (size_t i = 0; i < vecRoom_.size(); i++)
        {
            User *user = vecRoom_[i].getUser(ip);
            if (user != nullptr)
            {
                vecRoom_[i].removeUser(user);
                LOG_ERROR << "ChatService::clientQuitEcption, 用户下线";
                break;
            }
        }
    }
    else
    {
        LOG_ERROR << "ChatService::clientQuitEcption, 无此用户";
    }
}