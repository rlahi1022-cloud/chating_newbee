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
            std::cout <<"입력 : ";
            std::cin >> choice;

            if (choice == 3) 
            {
                std::cout << "\n프로그램 종료\n";
                running = false;
                break;
            }

            if (choice == 1)
            {
                // 로그인 정보 입력
                std::cout <<"===============LOGIN===============\n";
                std::cout << "ID: "; 
                std::cin >> id;
                std::cout << "PW: "; 
                std::cin >> pw;

                // test용 변수
                // login_in = true;
                // break;

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
                    std::cout << "[완료] 로그인 성공!\n";
                    login_in = true;
                    break;
                } 
                else 
                {
                    std::cout << "[오류] 로그인 실패: " << res["payload"]["result"] << "\n";
                    continue;
                }
                break;
            }
            else if (choice == 2) // 회원가입 : 중복검사를 추가하기 위해 세부적으로 쪼개짐
            {
                std::string id, pw, pw_confirm, nickname;
                std::cout << "\n[ 회원가입을 시작합니다 ]\n";

                // 아이디 중복 검사 : 서버랑 소통필요
                while (true) 
                {
                    std::cout << "[입력] 사용할 ID: "; 
                    std::cin >> id;
                    
                    json id_pkt;
                    id_pkt["type"] = PKT_ID_CHECK_REQ;
                    id_pkt["payload"] = {{"id", id}};
                    
                    json res = request_to_server(sock, id_pkt); // 아이디 중복검사 송신 : 디비에 저장된 아이디를 검사함
                    
                    if (res["payload"]["result"] == RES_AVAILABLE) {
                        std::cout << ">> 사용 가능한 아이디입니다.\n";
                        break; //
                    }
                    std::cout << ">> [경고] 이미 사용 중인 아이디입니다.\n";
                }

                //비밀번호 입력 및 자체 검증 : 서버에서 하는건 비효율적
                while (true) 
                {
                    std::cout << "[입력] 비밀번호(영문+숫자 혼합): "; std::cin >> pw;
                    std::cout << "[입력] 비밀번호 확인: "; std::cin >> pw_confirm;

                    if (pw != pw_confirm) 
                    {
                        std::cout << ">> [오류] 비밀번호가 서로 일치하지 않습니다.\n";
                        continue;
                    }
                    // if (!is_valid_password(pw)) 
                    // {   //체크 함수
                    //     std::cout << ">> [오류] 영문과 숫자를 모두 포함해야 합니다.\n";
                    //     continue;
                    // }
                    break; // 비밀번호 통과, 닉네임 단계로
                }

                //닉네임 중복 검사  : 서버랑 소통해야됨
                while (true) 
                {
                    std::cout << "[입력] 사용할 닉네임: "; std::cin >> nickname;
                    
                    json nick_pkt;
                    nick_pkt["type"] = PKT_NICK_CHECK_REQ;
                    nick_pkt["payload"] = {{"nickname", nickname}};
                    
                    json res = request_to_server(sock, nick_pkt);
                    
                    if (res["payload"]["result"] == RES_AVAILABLE) 
                    {
                        std::cout << ">>[완료] 닉네임 생성완료!\n";
                        break; // 모든 검증 완료
                    }
                    std::cout << ">> [경고] 이미 사용 중인 닉네임입니다.\n";
                }

                // 최종 가입 요청
                json signup_packet; // 통신방신은 json으로 통일중
                signup_packet["type"] = PKT_SIGNUP_REQ;
                signup_packet["payload"] = {{"id", id}, {"pw", pw}, {"nickname", nickname}};
                
                json res = request_to_server(sock, signup_packet); // 주고+받기 대기

                if (res["payload"]["result"] == RES_SUCCESS) 
                {
                    std::cout << "\n[완료] 가입 성공! 이제 로그인해 주세요.\n";
                    break;
                } 
                else 
                {
                    std::cout << "\n[경고] 가입 실패: " << res["payload"]["result"] << "\n";
                }
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
                std::cout <<"개인 설정 \n";
                break; // 서버랑 통신해서 디비를 업데이트하는 쿼리를 적어야됨
            }
        }
    }

    close(sock);
    return 0;
}