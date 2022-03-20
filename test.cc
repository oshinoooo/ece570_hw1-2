#include <iostream>

#include "thread.h"

using namespace std;

void show(void* ptr) {
    for (int i = 0; i < 10; ++i)
        cout << (long)ptr << " ";
    cout << endl;
}

void start(void* ptr) {
    for (long i = 0; i < 3; ++i) {
        long index = i;
        thread_create(show, (void*)index);
    }
}

int main(int argc, char* argv[]) {
    thread_libinit(start, nullptr);
    return 0;
}
