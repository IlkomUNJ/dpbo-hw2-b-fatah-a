#pragma once
#include <string>
#include "utils.h"

struct Item {
    int id = -1;
    int seller_id = -1;
    std::string name;
    double price = 0;
    int stock = 0;

    std::string serialize() const {
        std::string nm = name;
        for(char &c: nm) if(c=='|') c='/';
        return std::to_string(id) + "|" + std::to_string(seller_id) + "|" + nm + "|" + std::to_string(price) + "|" + std::to_string(stock);
    }
    static Item deserialize(const std::string &line){
        Item it;
        auto p = split_line(line);
        if(p.size()>=5){
            it.id = std::stoi(p[0]);
            it.seller_id = std::stoi(p[1]);
            it.name = p[2];
            it.price = std::stod(p[3]);
            it.stock = std::stoi(p[4]);
        }
        return it;
    }
};
