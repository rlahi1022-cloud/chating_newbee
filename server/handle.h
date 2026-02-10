#ifndef HANDLE_H
#define HANDLE_H

#include <string>
#include <memory>
#include <nlohmann/json.hpp>

void handle_packet(std::shared_ptr<ChatClient> client, const std::string& msg);
void client_handler(int client_sock);

#endif