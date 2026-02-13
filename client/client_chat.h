#pragma once
#include <thread>

extern std::atomic<bool> chat_active; 
extern std::atomic <bool> receiver_started;
extern std::mutex cout_mutex;
void start_chat_receiver(int sock);
void stop_chat_receiver();