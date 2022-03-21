#include <iostream>
#include <queue>
#include <map>
#include <ucontext.h>

#include "thread.h"
#include "interrupt.h"

using namespace std;

enum lock_status {FREE, BUSY};
enum thread_status {RUNNING, FINISHED};

struct thread_control_block {
public:
    thread_control_block() : status(RUNNING), ucontext_ptr(nullptr) {}
    thread_control_block(thread_status _status) : status(_status), ucontext_ptr(nullptr) {}
    thread_control_block(thread_status _status, ucontext_t* _ucontext_ptr) : status(_status), ucontext_ptr(_ucontext_ptr) {}

public:
    thread_status status;
    ucontext_t* ucontext_ptr;
};

static bool lib_initialized = false;
thread_control_block* running_thread_ptr;

queue<thread_control_block*> ready_que;
map<unsigned int, thread_control_block*> lock_holder;
map<unsigned int, queue<thread_control_block*>> lock_que;
map<unsigned int, queue<thread_control_block*>> cond_que;

void release(thread_control_block* thread_ptr) {
    if (!thread_ptr)
        return;

    delete (char*) thread_ptr->ucontext_ptr->uc_stack.ss_sp;
    thread_ptr->ucontext_ptr->uc_stack.ss_sp = nullptr;
    thread_ptr->ucontext_ptr->uc_stack.ss_size = 0;
    thread_ptr->ucontext_ptr->uc_stack.ss_flags = 0;
    thread_ptr->ucontext_ptr->uc_link = nullptr;
    delete thread_ptr->ucontext_ptr;
    delete thread_ptr;
    thread_ptr = nullptr;
}

void thread_monitor(thread_startfunc_t func, void* arg, thread_control_block* thread_ptr) {
    interrupt_enable();

    func(arg);

    interrupt_disable();

    release(thread_ptr);
}

void scheduler() {
    while (!ready_que.empty()) {
//        cleanup();
        thread_control_block* thread_ptr = ready_que.front();
        ready_que.pop();
        swapcontext(thread_ptr->ucontext_ptr, running_thread_ptr->ucontext_ptr);
    }
}

int thread_libinit(thread_startfunc_t func, void* arg) {
    if (lib_initialized) {
        return -1;
    }

    lib_initialized = true;

    func(arg);

    scheduler();

    cout << "Thread library exiting." << endl;
    exit(0);
}

int thread_create(thread_startfunc_t func, void* arg) {
    interrupt_disable();

    if (!lib_initialized) {
        interrupt_enable();
        return -1;
    }

    thread_control_block* new_thread = new thread_control_block();

    new_thread->ucontext_ptr = new ucontext_t;
    getcontext(new_thread->ucontext_ptr);
    new_thread->ucontext_ptr->uc_stack.ss_sp    = new char[STACK_SIZE];
    new_thread->ucontext_ptr->uc_stack.ss_size  = STACK_SIZE;
    new_thread->ucontext_ptr->uc_stack.ss_flags = 0;
    new_thread->ucontext_ptr->uc_link           = nullptr;
    makecontext(new_thread->ucontext_ptr, (void (*)())thread_monitor, 3, func, arg, new_thread);

    ready_que.push(new_thread);

    interrupt_enable();

    return 0;
}

int thread_yield(void) {
    interrupt_disable();

    if (!lib_initialized) {
        interrupt_enable();
        return -1;
    }

    if (ready_que.empty()) {
        interrupt_enable();
        return 0;
    }

    ready_que.push(running_thread_ptr);

    thread_control_block* next_thread_ptr = ready_que.front();
    ready_que.pop();

    swapcontext(running_thread_ptr->ucontext_ptr, next_thread_ptr->ucontext_ptr);

    interrupt_enable();

    return 0;
}

int thread_lock(unsigned int lock) {
    interrupt_disable();

    if (!lib_initialized) {
        interrupt_enable();
        return -1;
    }

    if (lock_holder[lock] == running_thread_ptr) {
        interrupt_enable();
        return -1;
    }

    if (lock_holder[lock]) {
        lock_que[lock].push(running_thread_ptr);
        thread_control_block* next_thread_ptr = ready_que.front();
        ready_que.pop();
        swapcontext(running_thread_ptr->ucontext_ptr, next_thread_ptr->ucontext_ptr);
    }
    else {
        lock_holder[lock] = running_thread_ptr;
    }

    interrupt_enable();

    return 0;
}

int thread_unlock(unsigned int lock) {
    interrupt_disable();

    if (!lib_initialized) {
        interrupt_enable();
        return -1;
    }

    if (!lock_holder[lock] ||
        lock_holder[lock] != running_thread_ptr) {
        interrupt_enable();
        return -1;
    }

    lock_holder[lock] = nullptr;

    if (!lock_que.empty()) {
        thread_control_block* next_thread_ptr = lock_que[lock].front();
        lock_que[lock].pop();
        lock_holder[lock] = next_thread_ptr;
        ready_que.push(next_thread_ptr);
    }

    interrupt_enable();

    return 0;
}

int thread_wait(unsigned int lock, unsigned int cond) {
    interrupt_disable();

    if (!lib_initialized) {
        interrupt_enable();
        return -1;
    }

    if (!lock_holder[lock] ||
        lock_holder[lock] != running_thread_ptr) {
        interrupt_enable();
        return -1;
    }

    lock_holder[lock] = nullptr;

    if (!lock_que.empty()) {
        thread_control_block* next_thread_ptr = lock_que[lock].front();
        lock_que[lock].pop();
        lock_holder[lock] = next_thread_ptr;
        ready_que.push(next_thread_ptr);
    }

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