#include <iostream>
#include <string>
#include <arpa/inet.h>
#include <unistd.h>
#include <nlohmann/json.hpp> // json을 쓰기위해서 필요한 라이브러리
#include "../server/proto.h"
#include "client_handle.h"

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

    // 서버 주소 설정 (리눅스 로컬 환경 기준)
    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // 서버 IP
    serv_addr.sin_port = htons(5003);                  // 서버 포트
    // 서버 연결
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) 
    {
        std::cerr << "서버 연결 실패" << std::endl;
        return 1;
    }
    std::cout << "서버에 연결되었습니다.\n";

    while(running)
    {
        // 여기는 회원가입/ 로그인만 반복 !
        while (running && !login_in)
        {
            std::cout <<" welcome test chating server \n";
            std::cout << " 메뉴 \n\n";
            std::cout << "1. login  2. 회원가입 3.종료 \n";
            std::cin >> choice;

            if (choice == 3) 
            {
                std::cout << "\n프로그램 종료";
                running = false;
                break;
            }

            if (choice == 1)
            {
                // 로그인 정보 입력
                std::cout <<"LOGIN";
                std::cout << "ID: "; std::cin >> id;
                std::cout << "PW: "; std::cin >> pw;

                // test용 변수
                login_in = true;
                break;

                // 서버 프로토콜에 맞게 JSON 패킷 생성
                json login_packet;
                login_packet["type"] = "LOGIN_REQ";
                login_packet["payload"] = 
                {
                    {"id", id},
                    {"pw", pw}
                };

                json res = request_to_server(sock, login_packet); // 수신 + 받기 함수 추가 

                // 결과 
                if (res["payload"]["result"] == "success") 
                {
                    std::cout << "로그인 성공!\n";
                    login_in = true;
                    break;
                } 
                else 
                {
                    std::cout << "로그인 실패: " << res["payload"]["result"] << "\n";
                    continue;
                }
                break;
            }
            else if (choice ==2)
            {
                std::cout << "\n[ SIGN UP ]\n";
                std::cout << "ID: "; std::cin >> id;
                std::cout << "PW: "; std::cin >> pw;
                std::cout << "NICKNAME: "; std::cin >> nickname;

                json signup_packet;
                signup_packet["type"] = "SIGNUP_REQ";
                signup_packet["payload"] = 
                {
                    {"id", id},
                    {"pw", pw},
                    {"nickname", nickname}
                };
                json res = request_to_server(sock, signup_packet);

                //결과
                if (res["payload"]["result"] == "success") 
                {
                    std::cout << "가입 성공! 로그인해 주세요.\n";
                } 

                else 
                {
                    std::cout << "가입 실패: " << res["payload"]["result"] << "\n";
                }
                continue;
            }
        }

        // 로그인/ 회원가입 반복문을 빠져나오면 채팅방 및 설정 메뉴 반복문 생성 예정
        while (login_in && running)
        {
            std::cout << " 메뉴 \n\n";
            std::cout << "1. 채팅방  2. 메시지 3. 개인설정 4. 로그아웃 5. 프로그램 종료 \n";
            std::cin >> choice;

            if (choice == 3)
            {
                login_in = false; // 전체 구조를 깨진않지만 로그인으로 돌아가는 변수
                std::cout << " LOG OUT \n";
            }
            if (choice == 4)
            {
                running = false; // 전체 구조를 깨는 변수
                std::cout << "program out";
                break;
            }

            // ============== 종료키는 누르면 바로 종료될수있게==============
            
            if (choice == 1)
            {
                std::cout << " 채팅방 목록 \n";
                break; // 아직 통신은 없음
            }
            else if (choice == 2)
            {
                std::cout << "메시지\n";
                break; // 아직 통신은 없음
            }
            else if (choice == 3)
            {
                std::cout <<"개인 설정 \n"
                break; // 서버랑 통신해서 디비를 업데이트하는 쿼리를 적어야됨
            }
        }
    }

    close(sock);
    return 0;
}