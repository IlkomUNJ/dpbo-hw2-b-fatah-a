#pragma once
#include <string>
#include <ctime>
#include <sstream>
#include <iomanip>

inline std::time_t now_ts(){ return std::time(nullptr); }

inline std::string ts_to_str(std::time_t t){
    std::tm tm;
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

inline std::time_t days_ago(int k){
    return now_ts() - (std::time_t)k * 24 * 3600;
}

inline std::time_t months_ago(int months){
    std::time_t t = now_ts();
    std::tm tm;
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    tm.tm_mon -= months;
    return std::mktime(&tm);
}

// simple split by delimiter
inline std::vector<std::string> split_line(const std::string &s, char d='|'){
    std::vector<std::string> out;
    std::string cur;
    for(char c: s){
        if(c==d){ out.push_back(cur); cur.clear(); }
        else cur.push_back(c);
    }
    out.push_back(cur);
    return out;
}
