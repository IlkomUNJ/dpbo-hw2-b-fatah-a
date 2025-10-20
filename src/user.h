#pragma once
#include <string>
#include "utils.h"

struct User {
    int id = -1;
    std::string username;
    std::string password;
    int role = 0; // 0 buyer,1 seller,2 admin
    int account_id = -1;

    std::string serialize() const {
        return std::to_string(id) + "|" + username + "|" + password + "|" + std::to_string(role) + "|" + std::to_string(account_id);
    }
    static User deserialize(const std::string &line){
        User u;
        auto p = split_line(line);
        if(p.size()>=5){
            u.id = std::stoi(p[0]);
            u.username = p[1];
            u.password = p[2];
            u.role = std::stoi(p[3]);
            u.account_id = std::stoi(p[4]);
        }
        return u;
    }
};
