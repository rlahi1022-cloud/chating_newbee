#include <string>
#include <set>
#include <vector>
#include <mutex>
#include <arpa/inet.h>
#include <unistd.h>
#include <nlohmann/json.hpp>
#include "chatingroom.h"
#include "proto.h"

using namespace nlohmann;
std::vector<ChatRoom> rooms(10);

/*===============================================
        class 전방선언으로 따로 class언급안함
==================================================*/

// 방 생성
bool ChatRoom::create(int creator_sock, const std::string& pw)
{
    std::lock_guard<std::mutex> lock(room_mutex);

    if (active)
        return false;

    active = true;
    password = pw;
    host_sock = creator_sock;
    members.insert(creator_sock);

    return true;
}

// 방 입장
bool ChatRoom::enter(int sock, const std::string& pw)
{
    // ===== 디버그 로그 (기존 멤버 변수 그대로 사용) =====
    std::cout << "[DEBUG] active: " << (active ? 1 : 0) << "\n";
    std::cout << "[DEBUG] saved password: [" << password << "]\n";
    std::cout << "[DEBUG] input  password: [" << pw << "]\n";
    std::cout << "[DEBUG] members size: " << members.size() << "\n";

        if (!active)
    {
        std::cout << "[DEBUG] fail reason: !active\n";
        return false;
    }

    if (password != pw)
    {
        std::cout << "[DEBUG] fail reason: password mismatch\n";
        return false;
    }

    if (members.size() >= 10)
    {
        std::cout << "[DEBUG] fail reason: members full\n";
        return false;
    }

    members.insert(sock);
    std::cout << "[DEBUG] enter success, members size now: " << members.size() << "\n";
    return true;
}

// 방 퇴장
// return
// 0 = 일반 퇴장
// 1 = 방장 승계 발생
// 2 = 방 삭제
int ChatRoom::leave(int sock, int& new_host)
{
    std::lock_guard<std::mutex> lock(room_mutex);

    new_host = -1;

    if (!active)
        return 0;

    members.erase(sock);

    if (members.empty())
    {
        reset();
        return 2;
    }

    if (sock == host_sock)
    {
        host_sock = *members.begin();
        new_host = host_sock;
        return 1;
    }

    return 0;
}
// 활성 여부
bool ChatRoom::is_active()
{
    std::lock_guard<std::mutex> lock(room_mutex);
    return active;
}
// 멤버 수
int ChatRoom::member_count()
{
    std::lock_guard<std::mutex> lock(room_mutex);
    return static_cast<int>(members.size());
}
// 방장 소켓
int ChatRoom::get_host()
{
    std::lock_guard<std::mutex> lock(room_mutex);
    return host_sock;
}
// 브로드캐스트
// void ChatRoom::broadcast(const std::string& message)
// {
//     std::vector<int> snapshot;

//     {
//         std::lock_guard<std::mutex> lock(room_mutex);

//         if (!active)
//             return;

//         snapshot.assign(members.begin(), members.end());
//     }

//     nlohmann::json pkt;
//     pkt["type"] = PKT_CHAT_MESSAGE;
//     pkt["payload"] = {
//         {"message", message}
//     };

//     std::string out = pkt.dump();

//     for (int s : snapshot)
//     {
//         send(s, out.c_str(), out.length(), 0);
//     }
// }
// 내부 초기화
void ChatRoom::reset()
{
    active = false;
    password.clear();
    host_sock = -1;
    members.clear();
}
void ChatRoom::broadcast(const std::string& message)
{
    std::vector<int> snapshot;
    {
        std::lock_guard<std::mutex> lock(room_mutex);
        if (!active) return;
        snapshot.assign(members.begin(), members.end());
    }
    for (int s : snapshot)
    {
        send(s, message.c_str(), message.length(), 0);
    }
}

