//
// Created by prostoichelovek on 08.06.19.
//

#ifndef VIDEOTRANS_THINGWORX_HPP
#define VIDEOTRANS_THINGWORX_HPP


#include <string>
#include <utility>
#include <vector>
#include <map>
#include <sstream>

#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>

#include "utils.hpp"

using namespace std;

class Thingworx {
public:
    string url;
    string thingName;
    string appKey;

    Thingworx(string url, string thingName, string appKey)
            : url(std::move(url)), thingName(std::move(thingName)), appKey(std::move(appKey)) {

    }

    string sendRequest(const string &collection, const string &name, const string &entity, const string &entityName,
                       const map<string, string> &data, const string &method = "post") {
        string addr;
        try {
            stringstream ss;
            stringstream addrSS;
            addrSS << url << "/Thingworx/" << collection << "/" << name << "/"
                   << entity << "/" << entityName << "?";
            for (const auto &d : data) {
                if (d.first.empty() || d.second.empty())
                    continue;
                addrSS << d.first << "=" << d.second << "&";
            }
            addrSS << "appKey=" << appKey << "&";
            addrSS << "method=" << method;
            addr = addrSS.str();

            curlpp::Easy r;
            r.setOpt(new curlpp::options::Url(addr));
            std::list<std::string> header;
            header.emplace_back("Accept: text/csv");
            r.setOpt(new curlpp::options::HttpHeader(header));
            r.setOpt(new curlpp::options::ConnectTimeout(10));
            ss << r;

            string received = ss.str();
            findAndReplaceAll(received, "result", "");
            findAndReplaceAll(received, "\"", "");
            findAndReplaceAll(received, "\n", "");
            findAndReplaceAll(received, "\r", "");
            return received;
        } catch (exception &e) {
            log(ERROR, "Cannot send request to Thingworx", name + "/" + collection + "/" + entity + "/" + entityName,
                "with url", addr, " :", e.what());
        }
    }

    string execService(const string &thing, const string &service,
                       const map<string, string> &data) {
        return sendRequest("Things", thing, "Services", service, data);
    }

    string execService(const string &service, const map<string, string> &data) {
        return execService(thingName, service, data);
    }

    // Argument [properties] can not be null. WTF?
    string getProperty(const string &thing, const string &property) {
        return sendRequest("Things", thing, "Properties", property, {{}}, "get");
    }

    string getProperty(const string &property) {
        return getProperty(thingName, property);
    }

    string setProperty(const string &thing, const string &property, const string &value) {
        string result = sendRequest("Things", thing, "Properties", property, {{"value", value}}, "put");
        if (!result.empty()) {
            log(ERROR, "Cannot set value for property", thing + "/" + property, "to", value, " :", result);
        }
        return result;
    }

    string setProperty(const string &property, const string &value) {
        return setProperty(thingName, property, value);
    }
};


#endif //VIDEOTRANS_THINGWORX_HPP
