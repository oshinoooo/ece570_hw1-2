#include <iostream>
#include <queue>
#include <map>
#include <unordered_map>
#include <ucontext.h>

#include "thread.h"
#include "interrupt.h"

using namespace std;

static bool lib_initialized = false;
queue<ucontext_t*> lock_ready;
queue<ucontext_t*> lock_waiting;
map<unsigned int, queue<ucontext_t*>> lock_signal;

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

    ucontext_t* ucontext_ptr;
    getcontext(ucontext_ptr);
    ucontext_ptr->uc_stack.ss_sp    = new char[STACK_SIZE];
    ucontext_ptr->uc_stack.ss_size  = STACK_SIZE;
    ucontext_ptr->uc_stack.ss_flags = 0;
    ucontext_ptr->uc_link           = nullptr;
    makecontext(ucontext_ptr, func, 1, arg);

    return 0;
}

int thread_yield(void) {
    interrupt_disable();

    if (!lib_initialized) {
        cout << "Thread library should be initialized first." << endl;
        return -1;
    }

    ucontext_t* ucontext_ptr;
    getcontext(ucontext_ptr);

    ucontext_t* next_ucontext_ptr = lock_ready.front();
    lock_ready.pop();

    swapcontext(ucontext_ptr, next_ucontext_ptr);

    interrupt_enable();

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