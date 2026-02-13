#include <iostream>
#include <arpa/inet.h>
#include <thread>
#include <atomic>
#include <nlohmann/json.hpp>
#include "../server/proto.h"
#include <mutex>

std::mutex cout_mutex;
using namespace nlohmann;

static std::atomic <bool> receiver_started(false); // 채팅활성상태
static std::atomic<bool> chat_active(false);

void start_chat_receiver(int sock)
{
    if (receiver_started)
        return;

    receiver_started = true;
    chat_active = true;

    std::thread([sock]()
    {
        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,
                   (const char*)&tv, sizeof(tv));

        char buffer[BUFFER_SIZE];
        std::string total;

        while (chat_active)
        {
            int len = recv(sock, buffer, BUFFER_SIZE - 1, 0);

            if (len < 0)
                continue;

            if (len == 0)
                break;

            buffer[len] = '\0';
            total += buffer;

            size_t pos;
            while ((pos = total.find('\n')) != std::string::npos)
            {
                std::string one_packet = total.substr(0, pos);
                total.erase(0, pos + 1);

                if (one_packet.empty())
                    continue;

                try
                {
                    auto pkt = json::parse(one_packet);

                    if (pkt.value("type", 0) == PKT_CHAT_MESSAGE)
                    {
                        auto payload = pkt["payload"];
                        std::string sender  = payload.value("sender", "");
                        std::string message = payload.value("message", "");

                        std::lock_guard<std::mutex> lock(cout_mutex);

                        // 현재 입력줄 지우기 (ANSI escape)
                        std::cout << "\33[2K\r";

                        // 채팅 로그 출력
                        std::cout << "[" << sender << "] : " << message << std::endl;

                        // 입력 프롬프트 다시 표시
                        std::cout << "[입력] : ";
                        std::cout.flush();
                    }
                }
                catch (...) {}
            }
        }
        receiver_started = false;
        chat_active = false;

    }).detach();
}
void stop_chat_receiver()
{
    chat_active = false;
}
