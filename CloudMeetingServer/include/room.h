#ifndef ROOM_H
#define ROOM_H

#include <unordered_map>
#include "public.h"
#include "user.h"
#include "json.hpp"
using json = nlohmann::json;

class Room
{
public:
    Room(const int &id) : m_id(id) {}
    void addUser(User *user)
    {
        if (user != nullptr)
        {
            m_users.insert({user->getIP(), user});
        }
        else
        {
            LOG_ERROR << "add User==nullptr";
        }
    }
    void removeUser(User *user)
    {
        for (auto it = m_users.begin(); it != m_users.end(); it++)
        {
            if (it->second == user)
            {
                m_users.erase(it);
                delete user;
                break;
            }
        }
        
    }
    User *getUser(uint32_t ip)
    {
        User *user = m_users.find(ip)->second;
        if (user != nullptr)
            return user;
        return nullptr;
    }
    uint getRoomId() { return m_id; }
    void setRoomId(uint id) { m_id = id; }
    json addUserMessage(User *sender)
    {
        json msg;
        msg["msgType"] = BROAD_ADDUSER_MESSAGE;
        msg["ip"] = sender->getIP();
        msg["roomUserNo"] = m_users.size();
        msg["roomId"] = m_id;
        return msg;
    }
    size_t getRoomUserNo() { return m_users.size(); }
    std::vector<uint32_t> getRoomUserIp() {
        std::vector<uint32_t> vecIp;
        for(auto it = m_users.begin(); it!=m_users.end(); it++)
        {
            vecIp.push_back(it->first);
        }
        return vecIp;
    }

private:
    std::multimap<uint32_t, User *> m_users;
    uint m_id;
};

#endif // ROOM_H