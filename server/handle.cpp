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
#include "handle.h"

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

// std:: shared_ptr<ChatClient> client :메모리 관리를 사람이 직접 하지 않고 시스템이 자동으로 하게 만들기 위해서 사용
// -> molloc 을 대체하는 기능 : std:: shcared_prt : 공유되는 포인터
// -> client 들이 전부 같은곳을 바라보게한다

// std::map<int, shared_ptr> : 
// c와 다르게 접속자를 찾을 때 for문을 돌며 빈자리를 찾을 필요 없음. 소켓 번호만 알면 바로 접근 가능

// json::parse(): sscanf 대신 이 한 줄로 모든 데이터를 읽음. 
// 데이터 형식이 바뀌어도 json을 사용한다면 변경 필요 x.

// 패킷 처리기 (JSON 기반)
void handle_packet(std::shared_ptr<ChatClient> client, const std::string& msg) 
{
    try //wile 문을 돌려야 계속 진행하지
    {
        json packet = json::parse(msg);
        int type = packet.value("type", 0); // -> 숫자로 입력 받게 해서 swich 문을 쓰게하자

        switch (type)
        {
            case PKT_LOGIN_REQ:
            {
                client->user_id = packet["payload"]["id"];
                std::cout << "[로그인] " << client->user_id << " 접속\n";
                
                // 응답도 JSON : 통일해야됨
                json res;
                res["type"] = "LOGIN_RES";
                res["payload"] = {{"result", "success"}};
                std::string out = res.dump();
                send(client->sock, out.c_str(), out.length(), 0);
            }
        break;
        }
    } 
    catch (const std::exception& e) 
    {
        std::cerr << "JSON 파싱 에러: " << e.what() << std::endl;
    }
}

void client_handler(int client_sock) 
{
    auto client = std::make_shared<ChatClient>(client_sock);
    
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients[client_sock] = client;
    }

    char buffer[1024];
    while (true) {
        int len = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
        if (len <= 0) break;
        
        buffer[len] = '\0';
        handle_packet(client, buffer);
    }

    // 종료 처리
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients.erase(client_sock);
    }
    std::cout << "클라이언트 접속 종료\n";
}