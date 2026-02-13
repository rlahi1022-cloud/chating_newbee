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
#include "chatingroom.h"

using namespace nlohmann;

std::shared_ptr <sql::Connection> conn = nullptr;
 // db 연결 포인터 : 통로를 연결해줘야됨 -> 초반에 열어두고 입력 검증에 반응해서 검증 및 데이터를 집어넣음 : 회원가입을 위해서 초기화를 시켜줌
// 접속자 명단 : c 배열 대신 map을 사용함
std:: map <int, std::shared_ptr<ChatClient>> clients;
std:: mutex clients_mutex; // 여러스레드가 동시에 접근할때 사용하는 뮤텍스

int main ()
{
    std::cout << "main 시작\n";
    try  // db 연결부분! : 처음에 열어놓기
    {
         std::cout << "DB 연결 시도 전\n";
        // C++ Connector 방식으로 DB 연결
        auto driver = sql::mariadb::get_driver_instance();
        sql::SQLString url("jdbc:mariadb://localhost:3306/chating_db");
        sql::Properties properties({{"user", "HI1022"}, {"password", "1234"}});
        
        // 전역변수 conn에 연결 객체 저장
        conn = std::shared_ptr<sql::Connection>(driver->connect(url, properties));

        std::cout << "- MariaDB C++ Connector 연결 성공!\n";
    } 
    catch (sql::SQLException& e) 
    {
        std::cerr << "- DB 연결 실패: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "- MariaDB 연결 성공!\n- 회원가입 준비 완료.\n";

    // 메인부분은 이전과 동일
    int server_sock = socket(PF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(SERVER_PORT);

    bind(server_sock, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_sock, 5);
    std::cout << "C++ JSON 서버 시작 (Port: 5003)...\n";

    while (true) 
    {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_len);
        
        // 스레드로 처리
        std::thread(client_handler, client_sock).detach();
    }
    close(server_sock);
    return 0;
}