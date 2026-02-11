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
#include <mariadb/conncpp.hpp>
#include "proto.h"
#include "handle.h"

using namespace nlohmann;
// extern std::shared_ptr <sql::Connection> conn = nullptr;

// 접속자 명단 : c 배열 대신 map을 사용함
extern std:: map <int, std::shared_ptr<ChatClient>> clients;
extern std:: mutex clients_mutex; // 여러스레드가 동시에 접근할때 사용하는 뮤텍스

// 아이디 // 닉네임 중복체크
#include "handle.h"
#include "proto.h" // 이미 정의된 RES_AVAILABLE, RES_DUPLICATE 사용
#include <iostream>

bool db_is_exists(const std::string& column, const std::string& value)
{
    try
    {
        //아이디/닉네임 로직 분리 및 실제 DB 컬럼명 매핑
        std::string safe_column;

        if (column == "user_id")
        {
            safe_column = "user_id"; 
        }
        else if (column == "nickname")
        {
            safe_column = "nickname";
        }
        else
        {
            std::cout << "허용되지 않은 컬럼: " << column << std::endl;
            return RES_FAIL;
        }

        //COUNT(*) 쿼리 실행
        std::string query = "SELECT COUNT(*) FROM users WHERE " + safe_column + " = ?";
        auto pstmt = conn->prepareStatement(query);
        pstmt->setString(1, value);

        auto res = pstmt->executeQuery();

        if (res->next()) 
        {
            int count = res->getInt(1);
            
            return (count == 0) ? RES_AVAILABLE : RES_DUPLICATE;
        }

        return RES_FAIL;
    }
    catch (sql::SQLException& e)
    {
        std::cout << "SQL 오류: " << e.what() << std::endl;
        return RES_FAIL;
    }
}

// 회원 정보를 DB에 집어넣는 함수
bool db_register_user(const std::string& id, const std::string& pw, const std::string& nick) 
{
    try 
    {
        auto pstmt = conn-> prepareStatement("INSERT INTO users (id, pw, nickname) VALUES (?, ?, ?)");
        pstmt->setString(1, id);
        pstmt->setString(2, pw);
        pstmt->setString(3, nick);
        pstmt->executeUpdate();
        return true;
    } 
    catch (sql::SQLException& e) 
    {
        return false;
    }
}

// 패킷 처리기 (JSON 기반)
void handle_packet(std::shared_ptr<ChatClient> client, const std::string& msg) 
{
    try 
    {
        json packet = json::parse(msg);
        int type = packet.value("type", 0);
        json res; // 응답용 JSON 객체 미리 생성

        switch (type)
        {
            case PKT_LOGIN_REQ:
            {
                res["type"] = PKT_LOGIN_RES;
                res["payload"] = {{"result", RES_SUCCESS}};
                break;
            }

            case PKT_ID_CHECK_REQ:
            {
                std::string id = packet["payload"]["id"];
                nlohmann::json res_pkt;
                res_pkt["type"] = PKT_ID_CHECK_RES;
                
                // db_is_exists가 false일 때 RES_AVAILABLE("available")이 나감
                res_pkt["payload"]["result"] = db_is_exists("user_id", id) ? RES_DUPLICATE : RES_AVAILABLE;
                
                std::string out = res_pkt.dump();
                send(client->sock, out.c_str(), out.length(), 0);
                break;
            }

            case PKT_NICK_CHECK_REQ:
            {
                res["type"] = PKT_NICK_CHECK_RES;
                //  닉네임도 같은 함수 재사용!
                std::string nick = packet["payload"]["nickname"];
                res["payload"]["result"] = db_is_exists("nickname", nick) ? RES_DUPLICATE : RES_AVAILABLE;
                break;
            }

            case PKT_SIGNUP_REQ:
            {
                res["type"] = PKT_SIGNUP_RES;
                bool success = db_register_user // 가입처리
                (
                    packet["payload"]["id"], 
                    packet["payload"]["pw"], 
                    packet["payload"]["nickname"]
                );
                res["payload"]["result"] = success ? RES_SUCCESS : RES_FAIL;
                break;
            }

            default: 
                return; // 알 수 없는 패킷은 전송하지 않고 종료
        }

        // 공통 응답 전송부: switch 문 밖에서 한 번만 처리하면 깔끔
        std::string out = res.dump();
        send(client->sock, out.c_str(), out.length(), 0);
        
    } 
    catch (const std::exception& e) 
    {
        std::cerr << "패킷 처리 에러: " << e.what() << std::endl;
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
