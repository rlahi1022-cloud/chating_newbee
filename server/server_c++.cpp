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
    int room_id = -1;

    ChatClient (int s) : sock(s) {}
    ~ChatClient ()
    {
        if (sock != -1)
        close(sock);
        std:: cout <<"클라이언트 객체 소멸\n";
    }
};

// 접속자 명단 : c 배열 대신 map을 사용함
std:: map <int, std::shared_ptr<ChatClient>> clients;
std:: mutex clients_mutex; // 여러스레드가 동시에 접근할때 사용하는 뮤텍스

// std:: shared_ptr<ChatClient> client :메모리 관리를 사람이 직접 하지 않고 시스템이 자동으로 하게 만들기 위해서 사용
// -> molloc 을 대체하는 기능 : std:: shcared_prt : 공유되는 포인터
// -> client 들이 전부 같은곳을 바라보게한다

// std::map<int, shared_ptr> : 
// c와 다르게 접속자를 찾을 때 for문을 돌며 빈자리를 찾을 필요 없음. 소켓 번호만 알면 바로 접근 가능

// json::parse(): sscanf 대신 이 한 줄로 모든 데이터를 읽음. 
// 데이터 형식이 바뀌어도 json을 사용한다면 변경 필요 x.

// 2. 패킷 처리기 (JSON 기반)
void handle_packet(std::shared_ptr<ChatClient> client, const std::string& msg) 
{
    try 
    {
        json packet = json::parse(msg);
        std::string type = packet.value("type", "");

        if (type == "LOGIN_REQ") 
        {
            client->user_id = packet["payload"]["id"];
            std::cout << "[로그인] " << client->user_id << " 접속\n";
            
            // 응답도 JSON으로
            json res;
            res["type"] = "LOGIN_RES";
            res["payload"] = {{"result", "success"}};
            std::string out = res.dump();
            send(client->sock, out.c_str(), out.length(), 0);
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