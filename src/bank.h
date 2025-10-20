#pragma once
#include <string>
#include <vector>
#include "utils.h"

struct BankAccount {
    int id = -1;
    int owner_user_id = -1;
    double balance = 0.0;
    std::time_t last_activity = 0;

    std::string serialize() const {
        return std::to_string(id) + "|" + std::to_string(owner_user_id) + "|" + std::to_string(balance) + "|" + std::to_string((long long)last_activity);
    }
    static BankAccount deserialize(const std::string &line){
        BankAccount a;
        auto p = split_line(line);
        if(p.size()>=4){
            a.id = std::stoi(p[0]);
            a.owner_user_id = std::stoi(p[1]);
            a.balance = std::stod(p[2]);
            a.last_activity = (std::time_t)std::stoll(p[3]);
        }
        return a;
    }
};

struct BankTx {
    int id = -1;
    int account_id = -1;
    double amount = 0;
    std::string type_;
    std::time_t ts = 0;

    std::string serialize() const {
        return std::to_string(id) + "|" + std::to_string(account_id) + "|" + std::to_string(amount) + "|" + type_ + "|" + std::to_string((long long)ts);
    }
    static BankTx deserialize(const std::string &line){
        BankTx b;
        auto p = split_line(line);
        if(p.size()>=5){
            b.id = std::stoi(p[0]);
            b.account_id = std::stoi(p[1]);
            b.amount = std::stod(p[2]);
            b.type_ = p[3];
            b.ts = (std::time_t)std::stoll(p[4]);
        }
        return b;
    }
};
