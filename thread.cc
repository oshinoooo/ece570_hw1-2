#include <iostream>
#include <queue>
#include <map>
#include <ucontext.h>

#include "thread.h"
#include "interrupt.h"

using namespace std;

enum status {FREE, BUSY};
static bool lib_initialized = false;
ucontext_t* scheduler_ucontext_ptr = new ucontext_t;

queue<ucontext_t*> ready_que;
map<unsigned int, queue<ucontext_t*>> lock_que;
map<unsigned int, queue<ucontext_t*>> cond_que;
map<unsigned int, status> lock_status;

struct TCB {
public:
    TCB() : thread_status(0), ucontext_ptr(nullptr) {}
    TCB(int _thread_status) : thread_status(_thread_status), ucontext_ptr(nullptr) {}
    TCB(int _thread_status, ucontext_t* _ucontext_ptr) : thread_status(_thread_status), ucontext_ptr(_ucontext_ptr) {}

public:
    int thread_status;
    ucontext_t* ucontext_ptr;
};

void join() {

}

void scheduler() {
    getcontext(scheduler_ucontext_ptr);

    while (!ready_que.empty()) {
        ucontext_t* next_ucontext_ptr = ready_que.front();
        ready_que.pop();
        swapcontext(next_ucontext_ptr, scheduler_ucontext_ptr);
    }
}

int thread_libinit(thread_startfunc_t func, void* arg) {
    if (!lib_initialized) {
        lib_initialized = true;
    }
    else {
//        cout << "Thread library should only be initialized once." << endl;
        return -1;
    }

    func(arg);

    scheduler();

    join();

    cout << "Thread library exiting." << endl;
    exit(0);
}

int thread_create(thread_startfunc_t func, void* arg) {
    interrupt_disable();

    if (!lib_initialized) {
//        cout << "Thread library should be initialized first." << endl;
        interrupt_enable();
        return -1;
    }

    ucontext_t* next_ucontext_ptr = new ucontext_t;
    getcontext(next_ucontext_ptr);
    next_ucontext_ptr->uc_stack.ss_sp    = new char[STACK_SIZE];
    next_ucontext_ptr->uc_stack.ss_size  = STACK_SIZE;
    next_ucontext_ptr->uc_stack.ss_flags = 0;
    next_ucontext_ptr->uc_link           = scheduler_ucontext_ptr;
    makecontext(next_ucontext_ptr, (void (*)())func, 1, arg);

    ready_que.push(next_ucontext_ptr);

    interrupt_enable();

    return 0;
}

int thread_yield(void) {
    interrupt_disable();

    if (!lib_initialized) {
//        cout << "Thread library should be initialized first." << endl;
        interrupt_enable();
        return -1;
    }

    if (ready_que.empty()) {
        interrupt_enable();
        return 0;
    }

    ucontext_t* curr_ucontext_ptr = new ucontext_t;
    getcontext(curr_ucontext_ptr);
    ready_que.push(curr_ucontext_ptr);

    ucontext_t* next_ucontext_ptr = ready_que.front();
    ready_que.pop();

    swapcontext(curr_ucontext_ptr, next_ucontext_ptr);

    interrupt_enable();

    return 0;
}

int thread_lock(unsigned int lock) {
    interrupt_disable();

    if (!lib_initialized) {
//        cout << "Thread library should be initialized first." << endl;
        interrupt_enable();
        return -1;
    }

    if (!lock_status.count(lock) || lock_status[lock] == FREE) {
        lock_status[lock] = BUSY;
    }
    else {
        ucontext_t* curr_ucontext_ptr = new ucontext_t;
        getcontext(curr_ucontext_ptr);
        lock_que[lock].push(curr_ucontext_ptr);

        ucontext_t* next_ucontext_ptr = ready_que.front();
        ready_que.pop();

        swapcontext(curr_ucontext_ptr, next_ucontext_ptr);
    }

    interrupt_enable();

    return 0;
}

int thread_unlock(unsigned int lock) {
    interrupt_disable();

    if (!lib_initialized) {
//        cout << "Thread library should be initialized first." << endl;
        interrupt_enable();
        return -1;
    }

    lock_status[lock] = FREE;

    if (!lock_que.empty()) {
        lock_status[lock] = BUSY;
        ucontext_t* next_ucontext_ptr = lock_que[lock].front();
        lock_que[lock].pop();
        ready_que.push(next_ucontext_ptr);
    }

    interrupt_enable();

    return 0;
}

int thread_wait(unsigned int lock, unsigned int cond) {
    interrupt_disable();

    if (!lib_initialized) {
//        cout << "Thread library should be initialized first." << endl;
        interrupt_enable();
        return -1;
    }

    thread_unlock(lock);

    ucontext_t* curr_ucontext_ptr = new ucontext_t;
    getcontext(curr_ucontext_ptr);
    cond_que[cond].push(curr_ucontext_ptr);

    ucontext_t* next_ucontext_ptr = ready_que.front();
    ready_que.pop();

    swapcontext(curr_ucontext_ptr, next_ucontext_ptr);

    thread_lock(lock);

    interrupt_enable();

    return 0;
}

int thread_signal(unsigned int lock, unsigned int cond) {
    interrupt_disable();

    if (!lib_initialized) {
//        cout << "Thread library should be initialized first." << endl;
        interrupt_enable();
        return -1;
    }

    if (!cond_que[cond].empty()) {
        ucontext_t* next_ucontext_ptr = cond_que[cond].front();
        cond_que[cond].pop();
        ready_que.push(next_ucontext_ptr);
    }

    interrupt_enable();

    return 0;
}

int thread_broadcast(unsigned int lock, unsigned int cond) {
    interrupt_disable();

    if (!lib_initialized) {
//        cout << "Thread library should be initialized first." << endl;
        interrupt_enable();
        return -1;
    }

    while (!cond_que[cond].empty()) {
        ucontext_t* next_ucontext_ptr = cond_que[cond].front();
        cond_que[cond].pop();
        ready_que.push(next_ucontext_ptr);
    }

    interrupt_enable();

    return 0;
}