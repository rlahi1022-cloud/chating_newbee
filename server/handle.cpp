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

// 접속자 명단 : c 배열 대신 map을 사용함
extern std:: map <int, std::shared_ptr<ChatClient>> clients;
extern std:: mutex clients_mutex; // 여러스레드가 동시에 접근할때 사용하는 뮤텍스



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