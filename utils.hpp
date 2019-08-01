//
// Created by prostoichelovek on 06.06.19.
//

#ifndef VIDEOTRANS_UTILS_HPP
#define VIDEOTRANS_UTILS_HPP

#include <iostream>
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>

using namespace std;

// https://stackoverflow.com/a/36763257/9577873
template<typename T>
class SharedQueue {
public:
    SharedQueue();

    ~SharedQueue();

    T &front();

    void pop_front();

    void push_back(const T &item);

    void push_back(T &&item);

    int size();

    bool empty();

private:
    std::deque<T> queue_;
    std::mutex mutex_;
    std::condition_variable cond_;
};

template<typename T>
SharedQueue<T>::SharedQueue() = default;

template<typename T>
SharedQueue<T>::~SharedQueue() = default;

template<typename T>
T &SharedQueue<T>::front() {
    std::unique_lock<std::mutex> mlock(mutex_);
    while (queue_.empty()) {
        cond_.wait(mlock);
    }
    return queue_.front();
}

template<typename T>
void SharedQueue<T>::pop_front() {
    std::unique_lock<std::mutex> mlock(mutex_);
    while (queue_.empty()) {
        cond_.wait(mlock);
    }
    queue_.pop_front();
}

template<typename T>
void SharedQueue<T>::push_back(const T &item) {
    std::unique_lock<std::mutex> mlock(mutex_);
    queue_.push_back(item);
    mlock.unlock();     // unlock before notificiation to minimize mutex con
    cond_.notify_one(); // notify one waiting thread

}

template<typename T>
void SharedQueue<T>::push_back(T &&item) {
    std::unique_lock<std::mutex> mlock(mutex_);
    queue_.push_back(std::move(item));
    mlock.unlock();     // unlock before notificiation to minimize mutex con
    cond_.notify_one(); // notify one waiting thread

}

template<typename T>
int SharedQueue<T>::size() {
    std::unique_lock<std::mutex> mlock(mutex_);
    int size = queue_.size();
    mlock.unlock();
    return size;
}

template<typename T>
bool SharedQueue<T>::empty() {
    return size() == 0;
}

string eraseSubStr(const std::string &mainStr, const std::string &toErase) {
    string res = mainStr;
    // Search for the substring in string
    size_t pos = res.find(toErase);
    if (pos != std::string::npos) {
        // If found then erase it from string
        res.erase(pos, toErase.length());
    }
    return res;
}

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
