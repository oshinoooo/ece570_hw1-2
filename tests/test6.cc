#include <iostream>

#include "thread.h"

using namespace std;

void func(void* ptr) {}

void test1() {
    cout << thread_libinit(func, nullptr) << endl;
    cout << thread_libinit(func, nullptr) << endl;
    cout << thread_libinit(func, nullptr) << endl;
    cout << thread_libinit(func, nullptr) << endl;
    cout << thread_libinit(func, nullptr) << endl;
}

void test2() {
    cout << thread_create(func, nullptr) << endl;
    cout << thread_create(func, nullptr) << endl;
    cout << thread_create(func, nullptr) << endl;
    cout << thread_create(func, nullptr) << endl;
    cout << thread_create(func, nullptr) << endl;
}

void test3() {
    cout << thread_yield() << endl;
    cout << thread_yield() << endl;
    cout << thread_yield() << endl;
    cout << thread_yield() << endl;
    cout << thread_yield() << endl;
}

void test4() {
    cout << thread_lock(1) << endl;
    cout << thread_lock(1) << endl;
    cout << thread_lock(1) << endl;
    cout << thread_lock(1) << endl;
    cout << thread_lock(1) << endl;
}

void test5() {
    cout << thread_unlock(1) << endl;
    cout << thread_unlock(1) << endl;
    cout << thread_unlock(1) << endl;
    cout << thread_unlock(1) << endl;
    cout << thread_unlock(1) << endl;
}

void test6() {
    cout << thread_wait(1, 1) << endl;
    cout << thread_wait(1, 1) << endl;
    cout << thread_wait(1, 1) << endl;
    cout << thread_wait(1, 1) << endl;
    cout << thread_wait(1, 1) << endl;
}

void test7() {
    cout << thread_signal(1, 1) << endl;
    cout << thread_signal(1, 1) << endl;
    cout << thread_signal(1, 1) << endl;
    cout << thread_signal(1, 1) << endl;
    cout << thread_signal(1, 1) << endl;
}

void test8() {
    cout << thread_broadcast(1, 1) << endl;
    cout << thread_broadcast(1, 1) << endl;
    cout << thread_broadcast(1, 1) << endl;
    cout << thread_broadcast(1, 1) << endl;
    cout << thread_broadcast(1, 1) << endl;
}

void test9() {
    cout << thread_unlock(1) << endl;
    cout << thread_yield() << endl;
    cout << thread_broadcast(1, 1) << endl;
    cout << thread_signal(1, 1) << endl;
    cout << thread_wait(1, 1) << endl;
}

void test10() {
    cout << thread_broadcast(1, 1) << endl;
    cout << thread_yield() << endl;
    cout << thread_yield() << endl;
    cout << thread_broadcast(1, 1) << endl;
    cout << thread_yield() << endl;
    cout << thread_broadcast(1, 1) << endl;
    cout << thread_signal(1, 1) << endl;
    cout << thread_wait(1, 1) << endl;
}

void start(void* ptr) {
    test1();
    test2();
    test3();
    test4();
    test5();
    test6();
    test7();
    test8();
    test9();
    test10();
}

int main() {
    cout << thread_libinit(start, nullptr) << endl;
    return 0;
}