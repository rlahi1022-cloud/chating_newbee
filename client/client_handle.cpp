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
    try
    {
        std::string send_msg = req_packet.dump() + "\n";
        send(sock, send_msg.c_str(), send_msg.length(), 0);

        std::string total;
        char buffer[BUFFER_SIZE];

        while (true)
        {
            int len = recv(sock, buffer, BUFFER_SIZE - 1, 0);
            if (len <= 0)
                throw std::runtime_error("서버 응답 없음");

            buffer[len] = '\0';
            total += buffer;

            size_t pos;
            while ((pos = total.find('\n')) != std::string::npos)
            {
                std::string one = total.substr(0, pos);
                total.erase(0, pos + 1);

                if (one.empty())
                    continue;

                json pkt = json::parse(one);

                int type = pkt.value("type", 0);

                //ENTER_RES일 때만 반환
                if (type == PKT_ROOM_ENTER_RES)
                {
                    return pkt;
                }

                // 채팅 메시지는 무시
                if (type == PKT_CHAT_MESSAGE)
                {
                    continue;
                }
                // 다른 응답이면 그냥 반환
                return pkt;
            }
        }
    }
    catch (...)
    {
        return { {"type", "ERROR"}, {"payload", {{"result", "fail"}}} };
    }
}
