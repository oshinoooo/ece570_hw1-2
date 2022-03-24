#include <iostream>
#include <vector>
#include <queue>
#include <map>
#include <limits.h>

#include "thread.h"

using namespace std;

unsigned int myLock = 0;
unsigned int cond = 1;

long current_position;
long max_disk_queue;
long number_of_requesters;
vector<queue<string>> requests;
map<long, string> buffer;

void printSchedulerState() {
    cout << "========================================" << endl;
    cout << "current_position      : " << current_position << endl;
    cout << "max_disk_queue        : " << max_disk_queue << endl;
    cout << "-------------------------" << endl;
    cout << "number_of_requesters  : " << number_of_requesters << endl;
    for (int i = 0; i < requests.size(); ++i)
        cout << "requests[" << i << "].size()    : " << requests[i].size() << endl;
    cout << "-------------------------" << endl;
    cout << "buffer.size()         : " << buffer.size() << endl;
    for (const pair<long, string>& tmp : buffer)
        cout << "requester_id: " << tmp.first << ", track: " << tmp.second << endl;
    cout << "----------------------------------------" << endl;
}

void init(int argc, char* argv[]) {
    current_position = 0;
    max_disk_queue = min(3, 5);
    number_of_requesters = 5;

    queue<string> tmp;
    tmp.push("53");
    tmp.push("785");
    requests.push_back(tmp);

    tmp = queue<string>();
    tmp.push("914");
    tmp.push("350");
    requests.push_back(tmp);

    tmp = queue<string>();
    tmp.push("827");
    tmp.push("567");
    requests.push_back(tmp);

    tmp = queue<string>();
    tmp.push("302");
    tmp.push("230");
    requests.push_back(tmp);

    tmp = queue<string>();
    tmp.push("631");
    tmp.push("11");
    requests.push_back(tmp);

//    printSchedulerState();
}

void sendRequest(void* ptr) {
    int ret = thread_lock(myLock);
    if (ret == -1) {
        cout << "Error in thread library." << endl;
    }

    long requester_id = (long)ptr;
    queue<string>& tracks = requests[requester_id];

    while (!tracks.empty()) {
        while(max_disk_queue <= buffer.size() || buffer.count(requester_id)) {
            ret = thread_wait(myLock, cond);
            if (ret == -1) {
                cout << "Error in thread library." << endl;
            }
        }

        buffer.insert({requester_id, tracks.front()});

        cout << "requester " << requester_id << " track " << tracks.front() << endl;

        tracks.pop();

        ret = thread_broadcast(myLock, cond);
        if (ret == -1) {
            cout << "Error in thread library." << endl;
        }

//        printSchedulerState();
    }

    ret = thread_unlock(myLock);
    if (ret == -1) {
        cout << "Error in thread library." << endl;
    }
}

void processRequest(void* ptr) {
    int ret = thread_lock(myLock);
    if (ret == -1) {
        cout << "Error in thread library." << endl;
    }

    while (max_disk_queue > 0 || !buffer.empty()) {
        while(max_disk_queue > buffer.size()) {
            ret = thread_wait(myLock, cond);
            if (ret == -1) {
                cout << "Error in thread library." << endl;
            }
        }

        long requester_id;
        long track;
        long min_distance = INT_MAX;
        for (const pair<long, string>& block : buffer) {
            long cur_distance = abs(current_position - stoi(block.second));
            if (cur_distance < min_distance) {
                min_distance = cur_distance;
                requester_id = block.first;
                track = stoi(block.second);
            }
        }

        current_position = track;

        cout << "service requester " << requester_id << " track " << track << endl;

        buffer.erase(requester_id);

        if (requests[requester_id].empty()) {
            --number_of_requesters;
            max_disk_queue = min(max_disk_queue, number_of_requesters);
        }

        ret = thread_broadcast(myLock, cond);
        if (ret == -1) {
            cout << "Error in thread library." << endl;
        }

//        printSchedulerState();
    }

    ret = thread_unlock(myLock);
    if (ret == -1) {
        cout << "Error in thread library." << endl;
    }
}

void startDiskScheduler(void* ptr) {
    int ret = thread_create(processRequest, nullptr);
    if (ret == -1) {
        cout << "Error in thread library." << endl;
    }

    for (long i = 0; i < requests.size(); ++i) {
        ret = thread_create(sendRequest, (void*)i);
        if (ret == -1) {
            cout << "Error in thread library." << endl;
        }
    }
}

int main(int argc, char* argv[]) {
    init(argc, argv);
    int ret = thread_libinit(startDiskScheduler, nullptr);
    if (ret == -1) {
        cout << "Error in thread library." << endl;
    }
    return 0;
}