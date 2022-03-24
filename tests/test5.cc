#include <iostream>

#include "thread.h"

using namespace std;

unsigned int lock = 1;
unsigned int cond = 1;

bool buffer = false;
int number = 10;

void put(void* ptr) {
    int ret;

    ret = thread_lock(lock);
    if (ret == -1) {
        cout << "Error in thread library." << endl;
    }

    while (number) {
        while (buffer) {
            ret = thread_wait(lock, cond);
            if (ret == -1) {
                cout << "Error in thread library." << endl;
            }
        }

        buffer = true;
        --number;
        cout << "put " << number << " into buffer." << endl;

        ret = thread_signal(lock, cond);
        if (ret == -1) {
            cout << "Error in thread library." << endl;
        }
    }

    ret = thread_unlock(lock);
    if (ret == -1) {
        cout << "Error in thread library." << endl;
    }
}

void get(void* ptr) {
    int ret;

    ret = thread_lock(lock);
    if (ret == -1) {
        cout << "Error in thread library." << endl;
    }

    while (number) {
        while (!buffer) {
            ret = thread_wait(lock, cond);
            if (ret == -1) {
                cout << "Error in thread library." << endl;
            }
        }

        buffer = false;
        cout << "get " << number << " from buffer." << endl;

        ret = thread_signal(lock, cond);
        if (ret == -1) {
            cout << "Error in thread library." << endl;
        }
    }

    ret = thread_unlock(lock);
    if (ret == -1) {
        cout << "Error in thread library." << endl;
    }
}

void start(void* ptr) {
    int ret;

    ret = thread_create(put, nullptr);
    if (ret == -1) {
        cout << "Error in thread library." << endl;
    }

    ret = thread_create(get, nullptr);
    if (ret == -1) {
        cout << "Error in thread library." << endl;
    }
}

int main(int argc, char* argv[]) {
    int ret;
    ret = thread_libinit(start, nullptr);
    if (ret == -1) {
        cout << "Thread library initialization failed." << endl;
    }
    return 0;
}