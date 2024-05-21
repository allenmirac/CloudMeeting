#ifndef USER_H
#define USER_H

#include <string>

class User
{
public:
    User(uint32_t ip = 0, uint roomId = 0) : m_ip(ip), m_roomId(roomId) {}
    void setIP(uint32_t ip) { m_ip = ip; };
    void setRoomId(uint roomId) { m_roomId = roomId; }
    uint32_t getIP() { return m_ip; }
    uint getRoomId() { return m_roomId; }

private:
    uint32_t m_ip;
    uint m_roomId;
};

#endif // USER_H