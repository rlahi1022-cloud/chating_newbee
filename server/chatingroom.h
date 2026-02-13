#pragma once
#include <string>
#include <set>
#include <vector>
#include <mutex>
#include <arpa/inet.h>
#include <unistd.h>

class ChatRoom
{
    private:
        bool active = false;
        std::string password;
        int host_sock = -1;
        std::set<int> members;
        std::mutex room_mutex;

    public:
        ChatRoom() = default;

        bool create(int creator_sock, const std::string& pw);
        bool enter(int sock, const std::string& pw);
        int leave(int sock, int& new_host);
        bool is_active();
        int member_count();
        int get_host();
        void broadcast(const std::string& message);
        const std::set<int>& get_members() const;
        void reset();
};

extern std::vector<ChatRoom> rooms;