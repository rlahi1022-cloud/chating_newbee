#ifndef CLIENT_HANDLE_H
#define CLIENT_HANDLE_H

#include <string>
#include <memory>
#include <nlohmann/json.hpp>
using namespace nlohmann;

json request_to_server(int sock, const json& req_packet);
bool is_valid_password(const std::string& pw);

#endif