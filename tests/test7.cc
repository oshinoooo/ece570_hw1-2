#include <iostream>

#include "thread.h"

using namespace std;

unsigned int myLock = 1;
unsigned int cond = 1;

int number = 100;
bool buffer = false;

void put(void* ptr) {
    cout << thread_lock(1) << endl;

    while (number) {
        while (buffer) {
            thread_wait(myLock, cond);
        }

        --number;
        buffer = true;
        cout << "void put(void* ptr)" << endl;
        cout << thread_broadcast(myLock, cond) << endl;
    }

    cout << thread_unlock(1) << endl;
}

void get(void* ptr) {
    cout << thread_lock(1) << endl;

    while (number || buffer) {
        if (!buffer) {
            thread_wait(myLock, cond);
        }

        buffer = false;
        cout << "void get(void* ptr)" << endl;
        cout << thread_broadcast(myLock, cond) << endl;
    }

    cout << thread_unlock(1) << endl;
}

void start(void* ptr) {
    cout << thread_create(put, nullptr) << endl;
    cout << thread_create(get, nullptr) << endl;
    cout << thread_create(get, nullptr) << endl;
    cout << thread_create(get, nullptr) << endl;
    cout << thread_create(get, nullptr) << endl;
    cout << thread_create(get, nullptr) << endl;
}

int main() {
    cout << thread_libinit(start, nullptr) << endl;
    return 0;
}