//
// Created by prostoichelovek on 08.06.19.
//

#ifndef VIDEOTRANS_DATAPROCESSING_HPP
#define VIDEOTRANS_DATAPROCESSING_HPP


#include <string>
#include <map>
#include <vector>

#include "utils.hpp"

using namespace std;

map<string, string> deserializeData(const string &data) {
    map<string, string> res;
    for (const string &part : split(data, ";")) {
        vector<string> data = split(part, ":");
        if (data.empty())
            continue;
        if (data.size() != 2) {
            log(ERROR, "Incorrect argument in command:'", part, "'");
            continue;
        }
        res[data[0]] = data[1];
    }
    return res;
}

string serealizeData(map<string, string> &data) {
    string res;
    for (auto &d : data)
        res += d.first + ":" + d.second + ";";
    return res;
}

map<string, string> merge(map<string, string> st1, map<string, string> st2) {
    map<string, string> res;
    for (auto &s1 : st1) {
        res[s1.first] = s1.second;
    }
    for (auto &s2 : st2) {
        res[s2.first] = s2.second;
    }
    return res;
}

#endif //VIDEOTRANS_DATAPROCESSING_HPP
