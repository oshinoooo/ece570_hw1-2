#include <iostream>
#include <queue>
#include <unordered_map>
#include <sys/ucontext.h>
//#include <ucontext.h>
#include "thread.h"
#include "interrupt.h"

using namespace std;

static bool lib_initialized = false;

queue<ucontext_t*> ready;
queue<ucontext_t*> waite;

int thread_libinit(thread_startfunc_t func, void* arg) {
    if (!lib_initialized) {
        lib_initialized = true;
    }
    else {
        cout << "Thread library should only be initialized once." << endl;
        return -1;
    }

    func(arg);

    cout << "Thread library exiting." << endl;
    exit(0);
}

int thread_create(thread_startfunc_t func, void* arg) {
    if (!lib_initialized) {
        cout << "Thread library should be initialized first." << endl;
        return -1;
    }

    ucontext_t new_context;


    return 0;
}

int thread_yield(void) {
    if (!lib_initialized) {
        cout << "Thread library should be initialized first." << endl;
        return -1;
    }






    return 0;
}

int thread_lock(unsigned int lock) {
    if (!lib_initialized) {
        cout << "Thread library should be initialized first." << endl;
        return -1;
    }






    return 0;
}

int thread_unlock(unsigned int lock) {
    if (!lib_initialized) {
        cout << "Thread library should be initialized first." << endl;
        return -1;
    }





    return 0;
}

int thread_wait(unsigned int lock, unsigned int cond) {
    if (!lib_initialized) {
        cout << "Thread library should be initialized first." << endl;
        return -1;
    }





    return 0;
}

int thread_signal(unsigned int lock, unsigned int cond) {
    if (!lib_initialized) {
        cout << "Thread library should be initialized first." << endl;
        return -1;
    }




    return 0;
}

int thread_broadcast(unsigned int lock, unsigned int cond) {
    if (!lib_initialized) {
        cout << "Thread library should be initialized first." << endl;
        return -1;
    }





    return 0;
}

void start_preemptions(bool async, bool sync, int random_seed) {
    if (!lib_initialized) {
        cout << "Thread library should be initialized first." << endl;
        return;
    }





    return;
}