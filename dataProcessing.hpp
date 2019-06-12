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

#endif //VIDEOTRANS_DATAPROCESSING_HPP
