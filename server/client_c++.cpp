#include <iostream>
#include <string>
#include <arpa/inet.h>
#include <unistd.h>
#include <nlohmann/json.hpp> // json을 쓰기위해서 필요한 라이브러리

using json = nlohmann::json;

int main() 
{
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

    // 로그인 정보 입력
    std::string id, pw;
    std::cout << "ID: "; std::cin >> id;
    std::cout << "PW: "; std::cin >> pw;

    // 서버 프로토콜에 맞게 JSON 패킷 생성
    json login_packet;
    login_packet["type"] = "LOGIN_REQ";
    login_packet["payload"] = 
    {
        {"id", id},
        {"pw", pw}
    };

    // JSON을 문자열로 변환(dump)하여 전송
    std::string send_msg = login_packet.dump();
    send(sock, send_msg.c_str(), send_msg.length(), 0);

    // 7. 서버 응답 수신
    char buffer[1024];
    int len = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (len > 0) 
    {
        buffer[len] = '\0';
        try {
            json res = json::parse(buffer);
            if (res["type"] == "LOGIN_RES") {
                std::string result = res["payload"]["result"];
                std::cout << "로그인 결과: " << result << std::endl;
                
                if (result == "success") {
                    std::cout << "메인 메뉴로 진입합니다...\n";
                    // 여기에 메뉴 로직 추가 가능
                }
            }
        } 
        catch (const std::exception& e) 
        {
            std::cerr << "응답 파싱 에러: " << e.what() << std::endl;
        }
    }

    close(sock);
    return 0;
}