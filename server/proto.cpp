#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <mutex>
#include <mysql.h>
#include "proto.h"
#include "handle.h" // 클래스 정의가 끝나야 핸들함수 쓸수있음
// #include "handle.cpp" // 클래스 정의가 끝나야 핸들함수 쓸수있음

using namespace nlohmann;

//=================================================================
class ChatClient // 클라이언트 정보를 담는소켓 : c++에서는 클래스에 자동 소멸까지 담을 수있음
{
    public:
    int sock;
    std:: string user_id;
    std:: string nickname;
    int room_id = -1;
    int state = 0;

    ChatClient (int s) : sock(s) {}

    void update_from_json(const nlohmann::json& payload) 
    {
        if (payload.contains("id")) user_id = payload["id"];
        if (payload.contains("nickname")) nickname = payload["nickname"];
    }

    ~ChatClient ()
    {
        if (sock != -1)
        close(sock);
        std::cout << "클라이언트 객체 소멸 (ID: " << user_id << ")\n";
    }
};