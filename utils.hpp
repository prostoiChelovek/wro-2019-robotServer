//
// Created by prostoichelovek on 06.06.19.
//

#ifndef VIDEOTRANS_UTILS_HPP
#define VIDEOTRANS_UTILS_HPP

#include <iostream>
#include <string>

using namespace std;

void findAndReplaceAll(std::string &data, const std::string &toSearch, const std::string &replaceStr) {
    // Get the first occurrence
    size_t pos = data.find(toSearch);

    // Repeat till end is reached
    while (pos != std::string::npos) {
        // Replace this occurrence of Sub String
        data.replace(pos, toSearch.size(), replaceStr);
        // Get the next occurrence from the current position
        pos = data.find(toSearch, pos + replaceStr.size());
    }
}

#endif //VIDEOTRANS_UTILS_HPP
