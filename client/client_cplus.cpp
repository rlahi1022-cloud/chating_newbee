#include <iostream>
#include <limits>
#include <string>
#include <arpa/inet.h>
#include <unistd.h>
#include <nlohmann/json.hpp> // json을 쓰기위해서 필요한 라이브러리
#include "../server/proto.h"
#include "client_handle.h"
#include "client_chat.h"

using namespace nlohmann;

// 클라이언트는 사용하지 않지만, proto.h의 extern 선언 때문에 형식을 맞춰줘야 함
// std::map<int, std::shared_ptr<ChatClient>> clients;
// std::mutex clients_mutex;
// MYSQL* conn = nullptr;
//==============================컴파일 오류로 인한 정의 : 해결

int main() 
{
    int choice;
    std::string id, pw, nickname;
    bool running = true;
    bool login_in = false;
    // 소켓 생성
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1) 
    {
        std::cerr << "소켓 생성 실패" << std::endl;
        return 1;
    }

    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // 서버 IP
    serv_addr.sin_port = htons(SERVER_PORT);                  // 서버 포트
    // 서버 연결
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) 
    {
        std::cerr << "서버 연결 실패" << std::endl;
        return 1;
    }
    std::cout << "===============================================================\n";
    std::cout << " 서버에 연결되었습니다.\n";
    std::cout << "===============================================================\n";

    while(running)
    {
        // 여기는 회원가입/ 로그인만 반복 !
        while (running && !login_in)
        {
            std::cout <<" \n welcome test chating server \n";
            std::cout << "===============================================================\n";
            std::cout << " 메뉴 \n";
            std::cout << "1. login  \n2. 회원가입 \n3. 종료 \n";
            std::cout <<"[입력] : ";
            std::cin >> choice;

            if (choice == 3) 
            {
                std::cout << "===============================================================\n";
                std::cout << "\n                    프로그램 종료\n";
                std::cout << "===============================================================\n";
                running = false;
                break;
            }
            if (choice == 1)
            {
                std::cout << "===============LOGIN===============\n";

                std::cout << "ID: ";
                std::cin >> id;

                std::cout << "PW: ";
                std::cin >> pw;

                json login_packet;

                login_packet["type"] = PKT_LOGIN_REQ;
                login_packet["payload"] =
                {
                    {"id", id},
                    {"pw", pw}
                };

                json res = request_to_server(sock, login_packet);

                std::string result = res["payload"]["result"];

                if (result == RES_SUCCESS)
                {
                    std::cout << "=========================================\n";
                    std::cout << "[완료] 로그인 성공!\n";

                    login_in = true;
                    break;
                }
                else
                {
                    std::cout << "================================================\n";
                    std::cout << "[오류] 로그인 실패\n";
                    continue;
                }
            }
            else if (choice == 2) // 회원가입 : 중복검사를 추가하기 위해 세부적으로 쪼개짐
            {
                std::string id, pw, pw_confirm, nickname;
                std::cout << "\n[ 회원가입을 시작합니다 ]\n";

                // 아이디 중복 검사 : 서버랑 소통필요
                while (true) 
                {
                    std::cout << "============================================\n";
                    std::cout << "[입력] 사용할 ID: "; 
                    std::cin >> id;
                    
                    json id_pkt;
                    id_pkt["type"] = PKT_ID_CHECK_REQ;
                    id_pkt["payload"] = {{"id", id}};
                    
                    json res = request_to_server(sock, id_pkt);
                    
                    if (res["payload"]["result"] == RES_AVAILABLE) 
                    {
                        std::cout << "=====================================\n";
                        std::cout << ">>  사용 가능한 아이디입니다.\n";
                        std::cout << "=====================================\n";
                        break;
                    }
                    std::cout << "==========================================\n";
                    std::cout << ">> [경고] 이미 사용 중인 아이디입니다.\n";
                    std::cout << "===========================================\n";
                }

                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

                //비밀번호 입력 및 자체 검증
                while (true)
                {
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                    std::cout << "===============================================\n";
                    std::cout << "[입력] 비밀번호(영문+숫자 혼합): ";
                    std::getline(std::cin, pw);

                    std::cout << "================================================\n";
                    std::cout << "[입력] 비밀번호 확인: ";
                    std::getline(std::cin, pw_confirm);

                    auto trim = [](std::string& s)
                    {
                        s.erase(0, s.find_first_not_of(" \n\r\t"));
                        s.erase(s.find_last_not_of(" \n\r\t") + 1);
                    };

                    trim(pw);
                    trim(pw_confirm);

                    if (pw != pw_confirm)
                    {
                        std::cout << ">> [오류] 비밀번호가 서로 일치하지 않습니다.\n";
                        continue;
                    }

                    if (!is_valid_password(pw))
                    {
                        std::cout << ">> [오류] 영문과 숫자를 모두 포함해야 합니다.\n";
                        continue;
                    }

                    break;
                }

                //닉네임 중복 검사
                while (true) 
                {
                    std::cout << "===============================================================\n";
                    std::cout << "[입력] 사용할 닉네임: "; std::cin >> nickname;
                    
                    json nick_pkt;
                    nick_pkt["type"] = PKT_NICK_CHECK_REQ;
                    nick_pkt["payload"] = {{"nickname", nickname}};
                    
                    json res = request_to_server(sock, nick_pkt);
                    
                    if (res["payload"]["result"] == RES_AVAILABLE) 
                    {
                        std::cout << ">>[완료] 닉네임 생성완료!\n";
                        break;
                    }
                    std::cout << ">> [경고] 이미 사용 중인 닉네임입니다.\n";
                }

                // 최종 가입 요청
                json signup_packet;
                signup_packet["type"] = PKT_SIGNUP_REQ;
                signup_packet["payload"] = {{"id", id}, {"pw", pw}, {"nickname", nickname}};
                
                json res = request_to_server(sock, signup_packet);

                if (res["payload"]["result"] == RES_SUCCESS) 
                {
                    std::cout << "===============================================================";
                    std::cout << "\n[완료] 가입 성공! 이제 로그인해 주세요.\n";
                    std::cout << "===============================================================";
                    break;
                } 
                else 
                {
                    std::cout << "===============================================================";
                    std::cout << "\n[경고] 가입 실패: " << res["payload"]["result"] << "\n";
                    std::cout << "===============================================================";
                }
            }
        }

        // 로그인/ 회원가입 반복문을 빠져나오면 채팅방 및 설정 메뉴 반복문 생성 예정
        while (login_in && running)
        {
            std::cout << "============================\n";
            std::cout << "        메뉴\n";
            std::cout << "============================\n";
            std::cout << "1. 채팅방\n";
            std::cout << "2. 메시지\n";
            std::cout << "3. 개인 설정\n";
            std::cout << "4. 로그 아웃\n";
            std::cout << "5. 프로그램 종료 \n\n";
            std::cout << "[입력] : ";
            std::cin >> choice;

            if (choice == 4)
            {
                login_in = false; // 전체 구조를 깨진않지만 로그인으로 돌아가는 변수
                std::cout << "===============================================================\n";
                std::cout << "\n                LOG OUT \n";
                std::cout << "===============================================================\n";
            }
            if (choice == 5)
            {
                std::cout << "===============================================================\n";
                std::cout << "                      program out\n";
                std::cout << "===============================================================\n";
                running = false; // 전체 구조를 깨는 변수
            }

            // ============== 종료키는 누르면 바로 종료될수있게==============
            
            if (choice == 1)
            {
                json list_req; //생성

                list_req["type"] = PKT_ROOM_LIST_REQ; // 요청
                std::cout << "받은 type: " << list_req["type"] << std::endl;
                json res = request_to_server(sock, list_req);
                
                if (res["type"] == PKT_ROOM_LIST_RES)
                {
                    auto rooms = res["payload"]["rooms"];

                    std::cout << "=== 채팅방 목록 ===\n";

                    for (auto& r : rooms)
                    {
                        std::cout << "방 번호: " << r["room_id"]
                                << " | 활성화: " << (r["active"] ? "O" : "X")
                                << " | 인원: " << r["count"]
                                << "\n";
                    }
                    std::cout << "============================\n";
                    std::cout << "        chating 메뉴\n";
                    std::cout << "============================\n";
                    std::cout << "1. chating_room 생성";
                    std::cout << "2. chating_room 입장\n";
                    std::cout << "3. 뒤로가기 \n";
                    int sub; // 선택 담을 변수
                    std::cout <<"[입력]: ";
                    std::cin >> sub;
                    if (sub == 1)
                    {
                        std::string pw;
                        std::cout << "\n[입력] 비밀번호 입력: ";
                        std::cin >> pw;

                        // 앞뒤 공백 제거
                        pw.erase(0, pw.find_first_not_of(" \n\r\t"));
                        pw.erase(pw.find_last_not_of(" \n\r\t") + 1);

                        // 디버그 출력
                        std::cout << "클라 pw: [" << pw << "] length=" << pw.length() << "\n";

                        json create_req;
                        create_req["type"] = PKT_ROOM_CREATE_REQ;
                        create_req["payload"] = {{"password", pw}};

                        json create_res = request_to_server(sock, create_req);

                        if (create_res["payload"]["result"] == RES_SUCCESS)
                        {
                            int room_id = create_res["payload"]["room_id"];
                            std::cout << "===============================================================\n";
                            std::cout << "[안내] 방 생성 성공! \n 방 번호: " << room_id << "\n";
                            std::cout << "나가기는 exit 입력 해주세요.\n";
                            std::cout << "방 폭파는 /destroy 입니다";
                            start_chat_receiver(sock);
                            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                            while (true)
                            {
                                std::string msg;
                                {
                                    std::lock_guard<std::mutex> lock(cout_mutex);
                                    std::cout << "\r[입력] : ";
                                    std::cout.flush();
                                }

                                std::getline(std::cin, msg);
                                if (msg == "exit")
                                {
                                    json exit_req;
                                    exit_req["type"] = PKT_ROOM_EXIT_REQ;
                                    std::string out = exit_req.dump() + "\n";
                                    send(sock, out.c_str(), out.length(), 0);
                                    stop_chat_receiver();
                                    break;
                                }
                                if (msg == "/destroy")
                                {
                                    json destroy_req;
                                    destroy_req["type"] = PKT_ROOM_DESTROY_REQ;

                                    std::string out = destroy_req.dump() + "\n";
                                    send(sock, out.c_str(), out.length(), 0);

                                    stop_chat_receiver();
                                    break;
                                }
                                json chat_req;
                                chat_req["type"] = PKT_CHAT_MESSAGE;
                                chat_req["payload"] = {{"message", msg}};
                                std::string out = chat_req.dump() + "\n";
                                send(sock, out.c_str(), out.length(), 0);
                            }
                        }
                        else
                        {
                            std::cout << "===============================================================\n";
                            std::cout << "[경고] 방 생성 실패.\n";
                        }
                    }
                    else if (sub == 2)
                    {
                        int room_id;
                        std::string pw;
                        std::cout << "===============================================================\n";
                        std::cout << "\n[입력] 입장할 방 번호: ";
                        std::cin >> room_id;
                        std::cout << "===============================================================\n";
                        std::cout << "\n[입력] 비밀번호 입력: ";
                        std::cin >> pw;

                        json enter_req;
                        enter_req["type"] = PKT_ROOM_ENTER_REQ;
                        enter_req["payload"] =
                        {
                            {"room_id", room_id},
                            {"password", pw}
                        };
                        json enter_res = request_to_server(sock, enter_req);

                        if (enter_res["payload"]["result"] == RES_SUCCESS)
                        {
                            {
                                std::lock_guard<std::mutex> lock(cout_mutex);
                                std::cout << "[안내] 방 입장 성공! \n 방 번호: " << room_id << "\n";
                                std::cout << "나가기는 exit 입력 해주세요.\n";
                            }

                            start_chat_receiver(sock);
                            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

                            while (true)
                            {
                                std::string msg;

                                {
                                    std::lock_guard<std::mutex> lock(cout_mutex);
                                    std::cout << "[입력]: ";
                                }

                                std::getline(std::cin, msg);

                                if (msg == "exit")
                                {
                                    json exit_req;
                                    exit_req["type"] = PKT_ROOM_EXIT_REQ;

                                    std::string out = exit_req.dump() + "\n";
                                    send(sock, out.c_str(), out.length(), 0);

                                    stop_chat_receiver();
                                    break;
                                }

                                json chat_req;
                                chat_req["type"] = PKT_CHAT_MESSAGE;
                                chat_req["payload"] = {{"message", msg}};

                                std::string out = chat_req.dump() + "\n";
                                send(sock, out.c_str(), out.length(), 0);
                            }
                        }
                        else
                        {
                            std::cout << "===============================================================\n";
                            std::cout << "[경고] 입장 실패.\n";
                        }
                    }
                }
            }
            else if (choice == 2)
            {
                while (true)
                {
                    std::cout << "============================\n";
                    std::cout << " 메시지 메뉴\n";
                    std::cout << "============================\n";
                    std::cout << "1. 메시지 목록\n";
                    std::cout << "2. 메시지 보내기\n";
                    std::cout << "3. 뒤로가기\n";
                    std::cout << "입력: ";

                    int sub;
                    std::cin >> sub;

                    if (sub == 3)
                        break;

                    // ============================
                    // 1. 메시지 목록
                    // ============================
                    if (sub == 1)
                    {
                        json list_req;
                        list_req["type"] = PKT_MSG_LIST_REQ;

                        json res = request_to_server(sock, list_req);

                        if (res.value("type", 0) == PKT_MSG_LIST_RES &&
                            res["payload"].value("result", "") == RES_SUCCESS)
                        {
                            auto messages = res["payload"]["messages"];

                            if (messages.empty())
                            {
                                std::cout << "메시지가 없습니다.\n";
                                continue;
                            }

                            std::cout << "===== 메시지 목록 =====\n";

                            for (auto& m : messages)
                            {
                                std::cout << "번호: " << m.value("msg_id", 0)
                                        << " | 보낸사람: " << m.value("sender", "")
                                        << " | 읽음: "
                                        << (m.value("is_read", false) ? "O" : "X")
                                        << "\n";
                            }

                            std::cout << "읽을 메시지 번호 입력 (0 = 취소): ";
                            int msg_id;
                            std::cin >> msg_id;

                            if (msg_id == 0)
                                continue;

                            json read_req;
                            read_req["type"] = PKT_MSG_READ_REQ;
                            read_req["payload"] = { {"msg_id", msg_id} };

                            json read_res = request_to_server(sock, read_req);

                            if (read_res.value("type", 0) == PKT_MSG_READ_RES &&
                                read_res["payload"].value("result", "") == RES_SUCCESS)
                            {
                                std::cout << "===== 메시지 내용 =====\n";
                                std::cout << read_res["payload"]
                                                .value("content", "")
                                        << "\n";
                            }
                            else
                            {
                                std::cout << "메시지 읽기 실패\n";
                            }
                        }
                        else
                        {
                            std::cout << "목록 조회 실패\n";
                        }
                    }

                    // ============================
                    // 2. 메시지 보내기
                    // ============================
                    else if (sub == 2)
                    {
                        std::string receiver;
                        std::string content;

                        std::cout << "받는 사람 ID: ";
                        std::cin >> receiver;

                        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

                        std::cout << "메시지 내용: ";
                        std::getline(std::cin, content);

                        json send_req;
                        send_req["type"] = PKT_MSG_SEND_REQ;
                        send_req["payload"] =
                        {
                            {"receiver", receiver},
                            {"content", content}
                        };

                        json send_res = request_to_server(sock, send_req);

                        if (send_res.value("type", 0) == PKT_MSG_SEND_RES &&
                            send_res["payload"].value("result", "") == RES_SUCCESS)
                        {
                            std::cout << "메시지 전송 완료\n";
                        }
                        else
                        {
                            std::cout << "메시지 전송 실패\n";
                        }
                    }
                }
            }
            else if (choice == 3)
            {
                std::string new_nickname;

                std::cout << "[입력] 변경할 닉네임: ";
                std::cin >> new_nickname;

                json nick_change_packet;

                nick_change_packet["type"] = PKT_NICK_CHANGE_REQ; //변경요청
                nick_change_packet["payload"] =  // 상태 업데이트
                {
                    {"nickname", new_nickname}
                };

                json res = request_to_server(sock, nick_change_packet);

                if (res["payload"]["result"] == RES_SUCCESS)
                {
                    std::cout << "[완료] 닉네임이 변경되었습니다.\n";
                }
                else if (res["payload"]["result"] == RES_DUPLICATE) // 중복검사
                {
                    std::cout << "[경고] 이미 사용 중인 닉네임입니다.\n";
                }
                else
                {
                    std::cout << "[오류] 닉네임 변경 실패.\n";
                }
            }
        }
    }
    close(sock);
    return 0;
}