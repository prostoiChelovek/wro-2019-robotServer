//
// Created by prostoichelovek on 08.06.19.
//

#ifndef VIDEOTRANS_THINGWORX_HPP
#define VIDEOTRANS_THINGWORX_HPP


#include <string>
#include <vector>
#include <map>
#include <sstream>

#include <curlpp/cURLpp.hpp>
#include <curlpp/Options.hpp>

#include "utils.hpp"

using namespace std;

string sendData(const string &addr, const string &key, const string &thingName,
                const map<string, string> &data) {
    stringstream recvData;
    try {
        stringstream url;
        url << addr << "/Thingworx/Things/" << thingName << "/Services/post_data?";
        url << "appKey=" << key << "&";
        url << "method=post&";
        for (const auto &d : data)
            url << d.first << "=" << d.second << "&";
        string urlStr = url.str();
        if (urlStr[urlStr.size() - 1] == '&')
            urlStr.pop_back();

        recvData << curlpp::options::Url(urlStr);
    } catch (exception &e) {
        log(ERROR, "Cannot send data to thingworx:", e.what());
        recvData << "[\"\"]";
    }
    string recvStr = recvData.str();

    auto first = recvStr.find("[\"") + 2;
    auto last = recvStr.find("\"]");
    if (first == string::npos || last == string::npos) {
        log(ERROR, "Incorrect response from thingworx:", recvStr);
        return "";
    }
    string toSay = recvStr.substr(first, last - first);
    return toSay.empty() ? "" : toSay;
}

string sendCommand(const string &addr, const string &key, const string &thingName,
                   const map<string, string> &cmd) {
    stringstream recvData;
    try {
        stringstream url;
        url << addr << "/Thingworx/Things/" << thingName << "/Services/post_command?";
        url << "appKey=" << key << "&";
        url << "method=post&";
        for (const auto &d : cmd) {
            url << "comName=" << d.first << "&";
            if (!d.second.empty())
                url << "rawValue=" << d.second << "&";
        }
        string urlStr = url.str();
        if (urlStr[urlStr.size() - 1] == '&')
            urlStr.pop_back();
        cout << urlStr << endl;
        recvData << curlpp::options::Url(urlStr);
    } catch (exception &e) {
        log(ERROR, "Cannot send command to thingworx:", e.what());
        recvData << "[\"\"]";
    }
    string recvStr = recvData.str();

    auto first = recvStr.find("[\"") + 2;
    auto last = recvStr.find("\"]");
    if (first == string::npos || last == string::npos) {
        log(ERROR, "Incorrect response from thingworx:", recvStr);
        return "";
    }
    string toSay = recvStr.substr(first, last - first);
    return toSay.empty() ? "" : toSay;
}


#endif //VIDEOTRANS_THINGWORX_HPP
