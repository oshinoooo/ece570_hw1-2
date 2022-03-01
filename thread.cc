#include <iostream>
#include <ucontext.h>

#include "thread.h"

using namespace std;

// getcontext
// makecontext
// swapcontext



///*
// * Initialize a context structure by copying the current thread's context.
// */
//getcontext(ucontext_ptr);           // ucontext_ptr has type (ucontext_t *)
//
///*
// * Direct the new thread to use a different stack.  Your thread library
// * should allocate STACK_SIZE bytes for each thread's stack.
// */
//char *stack = new char [STACK_SIZE];
//ucontext_ptr->uc_stack.ss_sp = stack;
//ucontext_ptr->uc_stack.ss_size = STACK_SIZE;
//ucontext_ptr->uc_stack.ss_flags = 0;
//ucontext_ptr->uc_link = NULL;
//
///*
// * Direct the new thread to start by calling start(arg1, arg2).
// */
//makecontext(ucontext_ptr, (void (*)()) start, 2, arg1, arg2);

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