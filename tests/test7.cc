#include <iostream>

#include "thread.h"

using namespace std;

unsigned int lock = 1;

void show(void* ptr) {
    int ret;
    ret = thread_lock(lock);
    if (ret == -1) {
        cout << "Error in thread library." << endl;
    }

    for (int i = 0; i < 100; ++i)
        cout << (long)ptr << " ";
    cout << endl;

    ret = thread_unlock(lock);
    if (ret == -1) {
        cout << "Error in thread library." << endl;
    }
}

void start(void* ptr) {
    for (long i = 0; i < 5; ++i) {
        int ret;
        ret = thread_create(show, (void*)i);
        if (ret == -1) {
            cout << "Error in thread library." << endl;
        }
    }
}

int main(int argc, char* argv[]) {
    int ret;
    ret = thread_yield();
    if (ret == -1) {
        cout << "Error in thread library." << endl;
    }

    ret = thread_libinit(start, nullptr);
    if (ret == -1) {
        cout << "Thread library initialization failed." << endl;
    }
    return 0;
}