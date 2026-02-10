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
// #include "handle.cpp" // 클래스 정의가 끝나야 핸들함수 쓸수있음
#include "handle.h" 
using namespace nlohmann;

MYSQL* conn = nullptr; // db 연결 포인터 : 통로를 연결해줘야됨 -> 초반에 열어두고 입력 검증에 반응해서 검증 및 데이터를 집어넣음 : 회원가입을 위해서 초기화를 시켜줌

// 접속자 명단 : c 배열 대신 map을 사용함
std:: map <int, std::shared_ptr<ChatClient>> clients;
std:: mutex clients_mutex; // 여러스레드가 동시에 접근할때 사용하는 뮤텍스


int main ()
{
    conn = mysql_init(NULL); // db 연결용
    if (mysql_real_connect(conn, "localhost", "HI1022", "1234", "chating_db", 3306, NULL, 0) == NULL) 
    {
        std::cerr << "- DB 연결 실패: " << mysql_error(conn) << std::endl;
        return 1; // 실패하면 서버 종료
    }
    
    std::cout << "- MariaDB 연결 성공!\n- 회원가입 준비 완료.\n";

    // 메인부분은 이전과 동일
    int server_sock = socket(PF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(5003);

    bind(server_sock, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_sock, 5);
    std::cout << "C++ JSON 서버 시작 (Port: 5003)...\n";

    while (true) 
    {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_len);
        
        // C++ 스레드로 처리
        std::thread(client_handler, client_sock).detach();
    }
    close(server_sock);
    return 0;
}