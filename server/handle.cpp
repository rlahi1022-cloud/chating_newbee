#include <iostream>
#include <string>
#include <vector>
#include <openssl/sha.h>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <iomanip>
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
#include "chatingroom.h"

using namespace nlohmann;
// extern std::shared_ptr <sql::Connection> conn = nullptr;
extern std::shared_ptr <sql::Connection> conn;

// 접속자 명단 : c 배열 대신 map을 사용함
extern std:: map <int, std::shared_ptr<ChatClient>> clients;
extern std:: mutex clients_mutex; // 여러스레드가 동시에 접근할때 사용하는 뮤텍스

// ============================
// SHA-256 해시 함수
// ============================
std::string sha256(const std::string& input)
{
    unsigned char hash[SHA256_DIGEST_LENGTH];

    SHA256(reinterpret_cast<const unsigned char*>(input.c_str()),
           input.size(), hash);

    std::stringstream ss;

    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        ss << std::hex
           << std::setw(2)
           << std::setfill('0')
           << (int)hash[i];
    }

    return ss.str();
}

// 아이디 중복검사
bool db_is_exists(const std::string& column, const std::string& value)
{
     try
    {
        std::string safe_column;

        if (column == "user_id")
            safe_column = "user_id";
        else if (column == "nickname")
            safe_column = "nickname";
        else
            return true;  // 이상하면 중복 취급

        std::string query =
            "SELECT COUNT(*) FROM users WHERE " + safe_column + " = ?";

        auto pstmt = conn->prepareStatement(query);
        pstmt->setString(1, value);
        auto res = pstmt->executeQuery();

        if (res->next())
        {
            int count = res->getInt(1);
            return count > 0;  // true = 이미 존재
        }

        return true;
    }
    catch (...) //    (...)은 모든 타입의 예외
    {
        return true;
    }
}

// 회원 정보를 DB에 집어넣는 함수
/*const std::string&는 :이 문자열을 수정하지 않음 + 복사하지 않고 참조로 받음*/
bool db_register_user(const std::string& id,
                      const std::string& raw_pw,
                      const std::string& hash_pw,
                      const std::string& nick)
{
    try
    {
        auto pstmt = conn->prepareStatement(
            "INSERT INTO users (user_id, password, password_hash, nickname) "
            "VALUES (?, ?, ?, ?)"
        );

        pstmt->setString(1, id);         // user_id
        pstmt->setString(2, raw_pw);     // 평문 저장 (학습용)
        pstmt->setString(3, hash_pw);    // 해시 저장
        pstmt->setString(4, nick);       // 닉네임

        pstmt->executeUpdate();
        return true;
    }
    catch (sql::SQLException& e)
    {
        std::cout << "회원가입 SQL 오류: " << e.what() << std::endl;
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
        json res;

        switch (type)
        {
            case PKT_LOGIN_REQ:
            {
                std::cout << "\n로그인요청\n";

                res["type"] = PKT_LOGIN_RES;

                std::string id = packet["payload"]["id"];
                std::string pw = packet["payload"]["pw"];

                // 원본 pw 상태 출력
                std::cout << "pw raw: [" << pw << "] length=" << pw.length() << "\n";

                //모든 공백 제거 (테스트용)
                pw.erase(std::remove_if(pw.begin(), pw.end(), ::isspace), pw.end());

                std::cout << "pw after trim: [" << pw << "] length=" << pw.length() << "\n";

                try
                {
                    auto pstmt = conn->prepareStatement(
                        "SELECT nickname, password_hash FROM users WHERE user_id=?"
                    );

                    pstmt->setString(1, id);

                    auto rs = pstmt->executeQuery();

                    if (rs->next())
                    {
                        std::string stored_hash =
                            rs->getString("password_hash").c_str();

                        std::string input_hash = sha256(pw);

                        std::cout << "input_hash  : " << input_hash << "\n";
                        std::cout << "stored_hash : " << stored_hash << "\n";

                        if (input_hash == stored_hash)
                        {
                            client->user_id = id;
                            client->nickname =
                                rs->getString("nickname").c_str();
                            client->state = STATE_LOBBY;

                            res["payload"]["result"] = RES_SUCCESS;

                            std::cout << "로그인 성공 → 상태 변경\n";
                        }
                        else
                        {
                            res["payload"]["result"] = RES_FAIL;
                            std::cout << "해시 불일치\n";
                        }
                    }
                    else
                    {
                        res["payload"]["result"] = RES_FAIL;
                        std::cout << "아이디 없음\n";
                    }
                }
                catch (...)
                {
                    res["payload"]["result"] = RES_FAIL;
                    std::cout << "로그인 예외 발생\n";
                }

                break;
            }
            case PKT_ID_CHECK_REQ: //로그인중복확인요청
            {
                std::string id = packet["payload"]["id"];
                res["type"] = PKT_ID_CHECK_RES;
                std::cout <<"로그인 중복 확인요청 \n";
                res["payload"]["result"] =
                    db_is_exists("user_id", id)
                        ? RES_DUPLICATE : RES_AVAILABLE;
                break;
            }
            case PKT_NICK_CHECK_REQ:  // 닉네임 중복 확인 요청
            {
                std::cout <<"닉네임 중복 확인 요청 \n";
                std::string nick = packet["payload"]["nickname"];
                res["type"] = PKT_NICK_CHECK_RES;
                res["payload"]["result"] =
                    db_is_exists("nickname", nick)
                        ? RES_DUPLICATE
                        : RES_AVAILABLE;
                break;
            }
            case PKT_SIGNUP_REQ: // 회원가입 요청
            {
                std::cout <<"회원가입 요청 \n";
                res["type"] = PKT_SIGNUP_RES;
                std::string id  = packet["payload"]["id"];
                std::string pw  = packet["payload"]["pw"];
                std::string nick = packet["payload"]["nickname"];

                std::string hash_pw = sha256(pw);

                bool success = db_register_user(id, pw, hash_pw, nick);

                res["payload"]["result"] = success ? RES_SUCCESS : RES_FAIL;
                break;
            }
            case PKT_NICK_CHANGE_REQ: // 닉네임 변경 요청
            {
                std::cout <<"닉네임 변경 요청 \n";
                std::cout <<"현재 상태: " << client->state << std::endl;
                res["type"] = PKT_NICK_CHANGE_RES;
                //로그인 상태 확인
                if (client->state != STATE_LOBBY)
                {
                    res["payload"]["result"] = RES_FAIL;
                    break;
                }
                std::string new_nick = packet["payload"]["nickname"];
                //동일 닉네임 방지
                if (new_nick == client->nickname)
                {
                    res["payload"]["result"] = RES_FAIL;
                    break;
                }
                // 중복 검사 재사용
                if (db_is_exists("nickname", new_nick))
                {
                    res["payload"]["result"] = RES_DUPLICATE;
                    break;
                }
                // DB 업데이트
                auto pstmt = conn->prepareStatement
                (
                    "UPDATE users SET nickname=? WHERE user_id=?"
                );
                pstmt->setString(1, new_nick);
                pstmt->setString(2, client->user_id);
                pstmt->executeUpdate();
                // 세션 값도 업데이트
                client->nickname = new_nick;
                res["payload"]["result"] = RES_SUCCESS;
                break;
            }
            case PKT_ROOM_CREATE_REQ: // 방생성
            {
                std::cout <<"방생성 요청";
                res["type"] = PKT_ROOM_CREATE_RES;
                std::string pw = packet["payload"].value("password", "");

                bool created = false;
                int created_room_id = -1;
                for (int i = 0; i < 10; ++i)
                {
                    if (!rooms[i].is_active())
                    {
                        if (rooms[i].create(client->sock, pw))
                        {
                            created = true;
                            created_room_id = i + 1;
                            client->room_id = created_room_id;
                            client->state = STATE_CHAT;
                            break;
                        }
                    }
                }
                res["payload"]["result"] = created ? RES_SUCCESS : RES_FAIL;
                if (created)
                {
                    res["payload"]["room_id"] = created_room_id;
                }
                break;
            }
            case PKT_ROOM_LIST_REQ:  // 채팅방 목록 요청
            {
                std::cout << "채팅방 목록 요청\n";
                res["type"] = PKT_ROOM_LIST_RES;
                json arr = nlohmann::json::array();

                for (int i = 0; i < 10; ++i)
                {
                    json room_info; // josn으로 보낸다
                    room_info["room_id"] = i + 1;
                    room_info["active"] = rooms[i].is_active();
                    room_info["count"] = rooms[i].member_count();

                    arr.push_back(room_info);
                }
                res["payload"]["rooms"] = arr;
                res["payload"]["result"] = RES_SUCCESS;
                std::cout <<" 목록요청 성공\n";
                break;
            }
            case PKT_ROOM_ENTER_REQ: // 채팅방 입장 요청
            {
                std::cout <<"입장시도 \n";
                res["type"] = PKT_ROOM_ENTER_RES;

                int room_id = packet["payload"]["room_id"];
                std::string pw = packet["payload"]["password"];
                std::cout << "입장 시도 room_id: " << room_id << "\n";
                std::cout << "비번: [" << pw << "]\n";
                if (room_id < 1 || room_id > 10)
                {
                    std::cout << "입장 실패\n";
                    res["payload"]["result"] = RES_FAIL;
                    break;
                }
                if (!rooms[room_id - 1].is_active())
                {
                    std::cout << "enter() password: [" << pw << "]\n";
                    std::cout << "enter() input pw: [" << pw << "]\n";
                    std::cout <<"입장 실패\n";
                    res["payload"]["result"] = RES_FAIL;
                    break;
                }
                bool success = rooms[room_id - 1].enter(client->sock, pw);
                if (success)
                {
                    client->room_id = room_id;
                    client->state = STATE_CHAT;
                    res["payload"]["result"] = RES_SUCCESS;

                    json notice;
                    notice["type"] = PKT_CHAT_MESSAGE;
                    notice["payload"]["sender"] = "[SERVER]";
                    notice["payload"]["message"] = client-> nickname + " 님이 입장했습니다.";

                    rooms[client->room_id - 1].broadcast(notice.dump() + "\n");
                }
                else
                {
                    res["payload"]["result"] = RES_FAIL;
                }
                break;
            }
            case PKT_CHAT_MESSAGE:  //채팅 메시지 전송 : 클래스내부에 로직있음
            {
                if (client->state != STATE_CHAT) // 상태확인
                    break;

                int room_id = client->room_id;

                if (room_id < 1 || room_id > 10)
                    break;

                std::string msg = packet["payload"]["message"]; // 메시지 전송

                json out;
                out["type"] = PKT_CHAT_MESSAGE;

                std::string sender_name = client->nickname;

                // 방장 여부 확인
                if (rooms[room_id - 1].get_host() == client->sock)
                {
                    sender_name += " (방장)";
                }
                out["payload"]["sender"] = sender_name;
                out["payload"]["message"] = msg;

                rooms[room_id - 1].broadcast(out.dump() + "\n");
                break;
            }
            case PKT_ROOM_EXIT_REQ: // 클라이언트 나갈때 요청 : 방장이 승계됨
            {
                if (client->state != STATE_CHAT)
                    break;

                int room_id = client->room_id;

                int new_host = -1;
                int result = rooms[room_id - 1].leave(client->sock, new_host);

                client->room_id = -1;
                client->state = STATE_LOBBY;

                res["type"] = PKT_ROOM_EXIT_RES;
                res["payload"]["result"] = RES_SUCCESS;

                // 방장 승계 공지
                if (new_host != -1)
                {
                    nlohmann::json notice;
                    notice["type"] = PKT_CHAT_MESSAGE;
                    notice["payload"]["sender"] = "SERVER";
                    notice["payload"]["message"] = "[안내] 방장이 변경되었습니다.\n";

                    rooms[room_id - 1].broadcast(notice.dump()+ "\n");
                }
                break;
            }
            case PKT_ROOM_DESTROY_REQ:
            {
                std::cout << "방폭파 요청\n";

                res["type"] = PKT_ROOM_DESTROY_RES;

                if (client->state != STATE_CHAT)
                {
                    res["payload"]["result"] = RES_FAIL;
                    break;
                }

                int room_id = client->room_id;

                if (room_id < 1 || room_id > 10)
                {
                    res["payload"]["result"] = RES_FAIL;
                    break;
                }

                // 방장 확인
                if (rooms[room_id - 1].get_host() != client->sock)
                {
                    res["payload"]["result"] = RES_FAIL;
                    break;
                }

                // 폭파 공지 먼저 방송
                nlohmann::json notice;
                notice["type"] = PKT_CHAT_MESSAGE;
                notice["payload"]["sender"] = "SERVER";
                notice["payload"]["message"] = "[안내] 방이 폭파되었습니다.";

                rooms[room_id - 1].broadcast(notice.dump() + "\n");

                // 방 멤버 전부 로비로 상태 변경
                for (int sock : rooms[room_id - 1].get_members())
                {
                    auto it = clients.find(sock);
                    if (it != clients.end())
                    {
                        it->second->room_id = -1;
                        it->second->state = STATE_LOBBY;
                    }
                }
                rooms[room_id - 1].reset();

                res["payload"]["result"] = RES_SUCCESS;

                break;
            }
            // ===============================
            // 메시지 전송
            // ===============================
            case PKT_MSG_SEND_REQ:
            {
                res["type"] = PKT_MSG_SEND_RES;

                if (client->state != STATE_LOBBY)
                {
                    res["payload"]["result"] = RES_FAIL;
                    break;
                }

                auto payload = packet["payload"];

                std::string receiver = payload.value("receiver", "");
                std::string content  = payload.value("content", "");

                if (receiver.empty() || content.empty())
                {
                    res["payload"]["result"] = RES_FAIL;
                    break;
                }

                try
                {
                    auto pstmt = conn->prepareStatement(
                        "INSERT INTO messages (sender_id, receiver_id, content) VALUES (?, ?, ?)"
                    );

                    pstmt->setString(1, client->user_id);
                    pstmt->setString(2, receiver);
                    pstmt->setString(3, content);
                    pstmt->executeUpdate();

                    res["payload"]["result"] = RES_SUCCESS;
                }
                catch (...)
                {
                    res["payload"]["result"] = RES_FAIL;
                }

                break;
            }
            case PKT_WHISPER_REQ:
            {
                res["type"] = PKT_WHISPER_RES;

                if (client->state != STATE_CHAT)
                {
                    res["payload"]["result"] = RES_FAIL;
                    break;
                }

                std::string target_nick = packet["payload"]["target"];
                std::string message     = packet["payload"]["message"];

                int room_id = client->room_id;

                if (room_id < 1 || room_id > 10)
                {
                    res["payload"]["result"] = RES_FAIL;
                    break;
                }

                int target_sock = -1;

                // 같은 방 안에서 대상 찾기
                for (int sock : rooms[room_id - 1].get_members())
                {
                    auto it = clients.find(sock);
                    if (it != clients.end() &&
                        it->second->nickname == target_nick)
                    {
                        target_sock = sock;
                        break;
                    }
                }

                if (target_sock == -1)
                {
                    res["payload"]["result"] = RES_FAIL;
                    break;
                }

                json out;
                out["type"] = PKT_CHAT_MESSAGE;
                out["payload"]["sender"] = "[귓속말] " + client->nickname;
                out["payload"]["message"] = message;

                std::string send_msg = out.dump() + "\n";
                send(target_sock, send_msg.c_str(), send_msg.length(), 0);

                res["payload"]["result"] = RES_SUCCESS;
                break;
            }
            // ===============================
            // 메시지 목록 조회
            // ===============================
            case PKT_MSG_LIST_REQ:
            {
                res["type"] = PKT_MSG_LIST_RES;

                if (client->state != STATE_LOBBY)
                {
                    res["payload"]["result"] = RES_FAIL;
                    break;
                }

                json arr = json::array();

                try
                {
                    auto pstmt = conn->prepareStatement(
                        "SELECT id, sender_id, is_read, created_at FROM messages WHERE receiver_id=? ORDER BY created_at DESC"
                    );

                    pstmt->setString(1, client->user_id);

                    auto rs = pstmt->executeQuery();

                    while (rs->next())
                    {
                        json msg;
                        msg["msg_id"] = rs->getInt("id");
                        msg["sender"] = rs->getString("sender_id");
                        msg["is_read"] = rs->getBoolean("is_read");
                        msg["created_at"] = rs->getString("created_at");

                        arr.push_back(msg);
                    }

                    res["payload"]["messages"] = arr;
                    res["payload"]["result"] = RES_SUCCESS;
                }
                catch (...)
                {
                    res["payload"]["result"] = RES_FAIL;
                }

                break;
            }
            // ===============================
            // 메시지 읽기
            // ===============================
            case PKT_MSG_READ_REQ:
            {
                res["type"] = PKT_MSG_READ_RES;

                if (client->state != STATE_LOBBY)
                {
                    res["payload"]["result"] = RES_FAIL;
                    break;
                }

                int msg_id = packet["payload"].value("msg_id", 0);

                if (msg_id == 0)
                {
                    res["payload"]["result"] = RES_FAIL;
                    break;
                }

                try
                {
                    // 내용 가져오기
                    auto pstmt = conn->prepareStatement(
                        "SELECT content FROM messages WHERE id=? AND receiver_id=?"
                    );

                    pstmt->setInt(1, msg_id);
                    pstmt->setString(2, client->user_id);

                    auto rs = pstmt->executeQuery();

                    if (rs->next())
                    {
                        std::string content = rs->getString("content").c_str();

                        res["payload"]["content"] = content;
                        res["payload"]["result"] = RES_SUCCESS;

                        // 읽음 처리
                        auto update = conn->prepareStatement(
                            "UPDATE messages SET is_read=1 WHERE id=?"
                        );

                        update->setInt(1, msg_id);
                        update->executeUpdate();
                    }
                    else
                    {
                        res["payload"]["result"] = RES_FAIL;
                    }
                }
                catch (...)
                {
                    res["payload"]["result"] = RES_FAIL;
                }

                break;
            }
        }
        // 여기서 한 번만 전송 : 각 프로토마다 보낼시 꼬일수있음
        if (!res.empty()) // 채팅방용 수신처리 : 통신이 비었으면 반응 안하겠금!
        {
            std::string out = res.dump()+"\n";
            std::cout << "[SERVER SEND] ";
            // std::string send_msg = out + "\n";
            send(client->sock, out.c_str(), out.length(), 0);
        }
    }
    catch (const std::exception& error)
    {
        std::cerr << "패킷 처리 에러: " << error.what() << std::endl;
    }
}
// 소켓통신
void client_handler(int client_sock) 
{
    auto client = std::make_shared<ChatClient>(client_sock);
    std::cout << "클라이언트 접속\n";
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients[client_sock] = client;
    }

    char buffer[BUFFER_SIZE];
    while (true) 
    {
        int len = recv(client_sock, buffer, BUFFER_SIZE - 1, 0);
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
