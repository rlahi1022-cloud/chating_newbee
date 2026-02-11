#include <iostream>
#include <string>
#include <arpa/inet.h>
#include <unistd.h>
#include <nlohmann/json.hpp> // json을 쓰기위해서 필요한 라이브러리
#include "../server/proto.h"

using namespace nlohmann;

// json : 식별자에 그대로 써도 됨 : 라이브러리에서 만든 클래스 이름 그래서 자료형으로 쓸수있다!
json request_to_server(int sock, const json& req_packet) 
{
    try {
        // 여기서 직렬화(dump)를 처리해버림.
        std::string send_msg = req_packet.dump();
        
        //  전송
        if (send(sock, send_msg.c_str(), send_msg.length(), 0) <= 0) {
            throw std::runtime_error("데이터 전송 실패");
        }

        //  수신 
        char buffer[1024];
        int len = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (len <= 0) throw std::runtime_error("서버 응답 없음");

        buffer[len] = '\0';
        
        // 역직렬화(parse) 후 반환
        return json::parse(buffer);

    } 
    catch (const std::exception& e) 
    {
        std::cerr << "[통신 오류] " << e.what() << std::endl;
        return {{"type", "ERROR"}, {"payload", {{"result", "fail"}}}};
    }
}