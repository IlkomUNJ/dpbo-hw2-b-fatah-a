#pragma once
#include <string>
#include "utils.h"

struct Transaction {
    int id = -1;
    int buyer_id = -1;
    int seller_id = -1;
    int item_id = -1;
    int qty = 0;
    double amount = 0.0;
    std::string status; // "paid","complete","canceled"
    std::time_t ts = 0;

    std::string serialize() const {
        return std::to_string(id) + "|" + std::to_string(buyer_id) + "|" + std::to_string(seller_id) + "|" + std::to_string(item_id) + "|" + std::to_string(qty) + "|" + std::to_string(amount) + "|" + status + "|" + std::to_string((long long)ts);
    }
    static Transaction deserialize(const std::string &line){
        Transaction t;
        auto p = split_line(line);
        if(p.size()>=8){
            t.id = std::stoi(p[0]);
            t.buyer_id = std::stoi(p[1]);
            t.seller_id = std::stoi(p[2]);
            t.item_id = std::stoi(p[3]);
            t.qty = std::stoi(p[4]);
            t.amount = std::stod(p[5]);
            t.status = p[6];
            t.ts = (std::time_t)std::stoll(p[7]);
        }
        return t;
    }
};
