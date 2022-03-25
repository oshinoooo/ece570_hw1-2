#include<iostream>

#include"thread.h"

using namespace std;

int number = 10;

void put(void* ptr) {
    thread_lock(1);
    number++;
    cout << "void put(void* ptr)" <<endl;
    thread_unlock(1);
}

void get(void* ptr) {
    thread_lock(1);
    number--;
    cout << "void get(void* ptr)" << endl;
    thread_yield();
    thread_unlock(1);
}

void start(void* ptr) {
    cout << thread_create(put, ptr) << endl;
    cout << thread_create(get, ptr) << endl;
    cout << thread_create(get, ptr) << endl;
    cout << thread_create(get, ptr) << endl;
}

int main() {
    cout << thread_libinit(start, (void*)100) << endl;
    return 0;
}