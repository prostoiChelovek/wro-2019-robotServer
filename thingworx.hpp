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
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>

#include "utils.hpp"

using namespace std;

void sendData(const string &addr, const string &key, const string &thingName,
              const map<string, string> &data) {
    string recvStr;
    try {
        stringstream url;
        url << addr << "/Thingworx/Things/" << thingName << "/Services/post_data?";
        url << "appKey=" << key << "&";
        url << "method=post&";
        for (const auto &d : data) {
            if (!d.second.empty())
                url << d.first << "=" << d.second << "&";
        }

        string urlStr = url.str();
        if (urlStr[urlStr.size() - 1] == '&')
            urlStr.pop_back();

        curlpp::Easy r;
        r.setOpt(new curlpp::options::Url(urlStr));
        r.perform();
    } catch (exception &e) {
        log(ERROR, "Cannot send data to thingworx:", e.what());
    }
}

string sendCommand(const string &addr, const string &key, const string &thingName,
                   const map<string, string> &cmd) {
    string recvStr;
    stringstream ss;
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

        curlpp::Easy r;
        r.setOpt(new curlpp::options::Url(urlStr));
        std::list<std::string> header;
        header.emplace_back("Accept: text/csv");
        r.setOpt(new curlpp::options::HttpHeader(header));
        ss << r;
    } catch (exception &e) {
        log(ERROR, "Cannot send command to thingworx:", e.what());
    }
    recvStr = ss.str();

    findAndReplaceAll(recvStr, "result", "");
    findAndReplaceAll(recvStr, "\"", "");
    findAndReplaceAll(recvStr, "\n", "");
    findAndReplaceAll(recvStr, "\r", "");
    string toSay = recvStr;
    return toSay.empty() ? "" : toSay;
}

string sendSpeech(const string &addr, const string &key, const string &thingName,
                  const string &speech) {
    string recvStr;
    stringstream ss;
    try {
        stringstream url;
        url << addr << "/Thingworx/Things/" << thingName << "/Services/process_speech?";
        url << "appKey=" << key << "&";
        url << "method=post&";
        url << "speech=" << speech << "&";
        string urlStr = url.str();
        if (urlStr[urlStr.size() - 1] == '&')
            urlStr.pop_back();

        curlpp::Easy r;
        r.setOpt(new curlpp::options::Url(urlStr));
        std::list<std::string> header;
        header.emplace_back("Accept: text/csv");
        r.setOpt(new curlpp::options::HttpHeader(header));
        ss << r;
    } catch (exception &e) {
        log(ERROR, "Cannot send command to thingworx:", e.what());
    }
    recvStr = ss.str();

    findAndReplaceAll(recvStr, "result", "");
    findAndReplaceAll(recvStr, "\"", "");
    findAndReplaceAll(recvStr, "\n", "");
    findAndReplaceAll(recvStr, "\r", "");
    string toSay = recvStr;
    return toSay.empty() ? "" : toSay;
}

void getData(const string &addr, const string &key, const string &thingName,
             map<string, string> &data) {
    stringstream ss;
    try {
        stringstream url;
        url << addr << "/Thingworx/Things/" << thingName << "/Services/getData?";
        url << "appKey=" << key << "&";
        url << "method=post";
        string urlStr = url.str();
        curlpp::Easy r;
        r.setOpt(new curlpp::options::Url(urlStr));
        std::list<std::string> header;
        header.emplace_back("Accept: text/csv");
        r.setOpt(new curlpp::options::HttpHeader(header));
        ss << r;
        string result = ss.str();
        findAndReplaceAll(result, "result", "");
        findAndReplaceAll(result, "\"", "");
        findAndReplaceAll(result, "\n", "");
        findAndReplaceAll(result, "\r", "");
        for (string &s : split(result, ";")) {
            vector<string> cmd;
            for (string &c : split(s, ":")) {
                cmd.emplace_back(c);
            }
            if (cmd.size() == 2) {
                data[cmd[0]] = cmd[1];
            }
        }
    } catch (exception &e) {
        log(ERROR, "Cannot send command to thingworx:", e.what());
    }
}


#endif //VIDEOTRANS_THINGWORX_HPP
