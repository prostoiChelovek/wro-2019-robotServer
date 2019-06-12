//
// Created by prostoichelovek on 12.06.19.
//

#ifndef VIDEOTRANS_SPEECHPARSING_HPP
#define VIDEOTRANS_SPEECHPARSING_HPP


#include <string>
#include <map>
#include <vector>

#include "utils.hpp"

class SpeechParser {
public:
    vector<pair<string, vector<string>>> &keyWords;

    explicit SpeechParser(vector<pair<string, vector<string>>> &keyWords) : keyWords(keyWords) {

    }

    void addKeyWord(string name, vector<string> words) {
        keyWords.emplace_back(name, words);
    }

    void parseSpeech(const string &speech, map<string, string> &data) {
        vector<string> kws = getKeyWords(speech);
        if (kws.empty())
            return;
        if (kws[0] == "rain") {
            data["weather_rain"] = "";
        }
        if (kws[0] == "temp") {
            if (kws[1] == "inside")
                data["climate_temperature"] = "";
            else
                data["weather_temperature"] = "";
        }
        if (kws[0] == "hum") {
            if (kws[1] == "inside")
                data["climate_humidity"] = "";
            else
                data["weather_humidity"] = "";
        }
        if (kws[0] == "on") {
            if (kws[1] == "light")
                data["light"] = "3";
        }
        if (kws[0] == "off") {
            if (kws[1] == "light")
                data["light"] = "0";
        }
        if (kws[0] == "set") {
            int val = 0;
            unsigned long numStart;
            if (kws[1] == "hum")
                numStart = speech.find("влажность") + 19;
            if (kws[1] == "temp") {
                numStart = speech.find("температуру") + 23;
                if (numStart == string::npos)
                    numStart = speech.find("температурa") + 23;
            }
            string numStr = speech.substr(numStart);
            if (numStr.find("в ") != string::npos)
                numStr.replace(0, 3, "");
            vector<int> vals = string2int(numStr);
            if (kws[1] == "hum")
                data["climate_normal_humidity"] = to_string(vals[0]);
            if (kws[1] == "temp")
                data["climate_normal_temperature"] = to_string(vals[0]);
        }
    }

private:
    vector<string> getKeyWords(const string &str) {
        vector<string> res;
        for (auto &kw : keyWords) {
            for (const string &word : kw.second) {
                if (str.find(word) != string::npos)
                    res.emplace_back(kw.first);
            }
        }
        return res;
    }

    vector<int> string2int(const string &str) {
        vector<int> res;
        string temp = str;
        map<string, int> oneDgtNums = {{"ноль",   0},
                                       {"один",   1},
                                       {"два",    2},
                                       {"три",    3},
                                       {"четыре", 4},
                                       {"пять",   5},
                                       {"шесть",  6},
                                       {"семь",   7},
                                       {"восемь", 8},
                                       {"девять", 9}};
        map<string, int> twoDgtNums = {{"десять",       10},
                                       {"одинадцать",   11},
                                       {"двенадцать",   12},
                                       {"тринадцать",   13},
                                       {"четырнадцать", 14},
                                       {"пятнадцать",   15},
                                       {"шестнадцать",  16},
                                       {"семнадцать",   17},
                                       {"восемнадцать", 18},
                                       {"девятнадцать", 19},
                                       {"двадцать",     20},
                                       {"тридцать",     30},
                                       {"сорок",        40},
                                       {"пятьдесят",    50},
                                       {"шестьдесят",   60},
                                       {"семьдесят",    70},
                                       {"восемьдесят",  80},
                                       {"девяносто",    90}};
        vector<unsigned long> twoDgtNumsPos;
        for (const auto &s : twoDgtNums) {
            auto pos = temp.find(s.first);
            if (pos != string::npos) {
                temp.replace(pos, s.first.size(), to_string(s.second));
                twoDgtNumsPos.emplace_back(pos);
            }
        }
        vector<unsigned long> oneDgtNumsPos;
        for (const auto &s : oneDgtNums) {
            auto pos = temp.find(s.first);
            if (pos != string::npos) {
                temp.replace(pos, s.first.size(), to_string(s.second));
                oneDgtNumsPos.emplace_back(pos);
            }
        }

        for (const auto &pos : twoDgtNumsPos) {
            for (const auto &oPos : oneDgtNumsPos) {
                if (pos + 3 == oPos) {
                    temp.replace(pos + 1, pos + 2, "");
                }
            }
        }

        for (const string &s : split(temp, " ")) {
            try {
                res.emplace_back(stoi(s));
            } catch (...) {
                continue;
            }
        }

        return res;
    }
};


#endif //VIDEOTRANS_SPEECHPARSING_HPP
