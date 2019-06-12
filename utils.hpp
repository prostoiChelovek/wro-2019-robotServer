//
// Created by prostoichelovek on 06.06.19.
//

#ifndef VIDEOTRANS_UTILS_HPP
#define VIDEOTRANS_UTILS_HPP

#include <iostream>
#include <string>

using namespace std;

#define argStr(key, value) key << ":" << value << ";"
#define ifKeyInMap(map, key) if(map.count(key) > 0)
#define ifStrInVec(vec, str) if(find(vec.begin(), vec.end(), str) != vec.end())

enum LogType {
    INFO, WARNING, ERROR
};

// https://stackoverflow.com/a/16338804/9577873
template<typename T>
void coutMany(T t) {
    cout << t << " " << endl;
}

template<typename T, typename... Args>
void coutMany(T t, Args... args) { // recursive variadic function
    cout << t << " " << flush;
    coutMany(args...);
}

template<typename T>
void cerrMany(T t) {
    cerr << t << " " << endl;
}

template<typename T, typename... Args>
void cerrMany(T t, Args... args) { // recursive variadic function
    cerr << t << " " << flush;
    cerrMany(args...);
}

template<typename T, typename... Args>
void log(int type, T t, Args... args) {
    switch (type) {
        case INFO:
            cout << "INFO: ";
            coutMany(t, args...);
            break;
        case WARNING:
            cerr << "WARNING: ";
            cerrMany(t, args...);
            break;
        case ERROR:
            cerr << "ERROR: ";
            cerrMany(t, args...);
            break;
    };
}

vector<string> split(const string &str, const string &delim) {
    vector<string> parts;
    size_t start, end = 0;
    while (end < str.size()) {
        start = end;
        while (start < str.size() && (delim.find(str[start]) != string::npos)) {
            start++;  // skip initial whitespace
        }
        end = start;
        while (end < str.size() && (delim.find(str[end]) == string::npos)) {
            end++; // skip to end of word
        }
        if (end - start != 0) {  // just ignore zero-length strings.
            parts.push_back(string(str, start, end - start));
        }
    }
    return parts;
}

#endif //VIDEOTRANS_UTILS_HPP
