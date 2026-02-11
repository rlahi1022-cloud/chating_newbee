#ifndef HANDLE_H
#define HANDLE_H

#include <string>
#include <memory>
#include <nlohmann/json.hpp>
#include <mariadb/conncpp.hpp>
#include "proto.h"

bool db_is_exists(const std::string& column, const std::string& value);
bool db_register_user(const std::string& id, const std::string& pw, const std::string& nick);
void handle_packet(std::shared_ptr<ChatClient> client, const std::string& msg);
void client_handler(int client_sock);


#endif