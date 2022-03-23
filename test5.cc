#include <iostream>

#include "thread.h"

using namespace std;

unsigned int lock = 1;

void show(void* ptr) {
    thread_unlock(lock);
    thread_lock(lock);
    for (int i = 0; i < 100; ++i)
        cout << (long)ptr << " ";
    cout << endl;
    thread_unlock(lock);
}

void start(void* ptr) {
    for (long i = 0; i < 5; ++i) {
        thread_create(show, (void*)i);
    }
}

int main(int argc, char* argv[]) {
    int ret = thread_libinit(start, nullptr);
    if (ret == -1) {
        cout << "Thread library initialization failed." << endl;
    }
    else {
        cout << "Thread library initialization succeeded." << endl;
    }
    return 0;
}
