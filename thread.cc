#include <iostream>
#include <ucontext.h>

#include "thread.h"
#include "interrupt.h"

using namespace std;

int thread_libinit(thread_startfunc_t func, void *arg) {





    cout << "Thread library exiting." << endl;
    exit(0);
}

int thread_create(thread_startfunc_t func, void *arg) {
    return 0;
}

int thread_yield(void) {
    return 0;
}

int thread_lock(unsigned int lock) {







    return 0;
}

int thread_unlock(unsigned int lock) {






    return 0;
}

int thread_wait(unsigned int lock, unsigned int cond) {
    return 0;
}

int thread_signal(unsigned int lock, unsigned int cond) {
    return 0;
}

int thread_broadcast(unsigned int lock, unsigned int cond) {
    return 0;
}

void start_preemptions(bool async, bool sync, int random_seed) {
    return;
}