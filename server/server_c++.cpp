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

using json = nlohmann::json; 

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

// 접속자 명단 : c 배열 대신 map을 사용함
std:: map <int, std::shared_ptr<ChatClient>> clients;
std:: mutex clients_mutex; // 여러스레드가 동시에 접근할때 사용하는 뮤텍스

int main ()
{
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