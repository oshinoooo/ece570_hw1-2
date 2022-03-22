#include <iostream>
#include <queue>
#include <map>
#include <ucontext.h>

#include "thread.h"
#include "interrupt.h"

using namespace std;

struct thread_control_block {
public:
    thread_control_block() : ucontext_ptr(nullptr) {}
    thread_control_block(ucontext_t* _ucontext_ptr) : ucontext_ptr(_ucontext_ptr) {}

public:
    ucontext_t* ucontext_ptr;
};

static bool is_initialized = false;
static thread_control_block* main_thread_ptr;
static thread_control_block* running_thread_ptr;

static queue<thread_control_block*> ready_queue;
static map<unsigned int, thread_control_block*> lock_holder;
static map<unsigned int, queue<thread_control_block*>> lock_queue;
static map<pair<unsigned int, unsigned int>, queue<thread_control_block*>> cond_queue;

static void release(thread_control_block* thread_ptr) {
    if (!thread_ptr) {
        return;
    }

    thread_ptr->ucontext_ptr->uc_stack.ss_sp    = nullptr;
    thread_ptr->ucontext_ptr->uc_stack.ss_size  = 0;
    thread_ptr->ucontext_ptr->uc_stack.ss_flags = 0;
    thread_ptr->ucontext_ptr->uc_link           = nullptr;
    delete[] (char*)thread_ptr->ucontext_ptr->uc_stack.ss_sp;
    delete thread_ptr->ucontext_ptr;
    delete thread_ptr;
    thread_ptr = nullptr;
}

static void thread_monitor(thread_startfunc_t func, void* arg, thread_control_block* thread_ptr) {
    interrupt_enable();
    func(arg);
    interrupt_disable();

    // release the memory
    release(thread_ptr);

    // go back to main thread
    setcontext(main_thread_ptr->ucontext_ptr);
}

int thread_libinit(thread_startfunc_t func, void* arg) {
    if (is_initialized) {
        return -1;
    }

    is_initialized = true;

    // initialize the scheduler
    main_thread_ptr = new thread_control_block();
    main_thread_ptr->ucontext_ptr = new ucontext_t;
    getcontext(main_thread_ptr->ucontext_ptr);

    // call user's function
    func(arg);

    interrupt_disable();

    while (!ready_queue.empty()) {
        running_thread_ptr = ready_queue.front();
        ready_queue.pop();
        swapcontext(main_thread_ptr->ucontext_ptr, running_thread_ptr->ucontext_ptr);
    }

    // release the memory
    release(main_thread_ptr);

    cout << "Thread library exiting." << endl;

    exit(0);
}

int thread_create(thread_startfunc_t func, void* arg) {
    interrupt_disable();

    if (!is_initialized) {
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

    ready_queue.push(new_thread);

    interrupt_enable();

    return 0;
}

int thread_yield(void) {
    interrupt_disable();

    if (!is_initialized) {
        interrupt_enable();
        return -1;
    }

    ready_queue.push(running_thread_ptr);

    swapcontext(running_thread_ptr->ucontext_ptr, main_thread_ptr->ucontext_ptr);

    interrupt_enable();

    return 0;
}

int thread_lock(unsigned int lock) {
    interrupt_disable();

    if (!is_initialized) {
        interrupt_enable();
        return -1;
    }

    if (lock_holder[lock] == running_thread_ptr) {
        interrupt_enable();
        return -1;
    }

    if (lock_holder[lock] != nullptr) {
        lock_queue[lock].push(running_thread_ptr);
        swapcontext(running_thread_ptr->ucontext_ptr, main_thread_ptr->ucontext_ptr);
    }
    else {
        lock_holder[lock] = running_thread_ptr;
    }

    interrupt_enable();

    return 0;
}

int thread_unlock(unsigned int lock) {
    interrupt_disable();

    if (!is_initialized) {
        interrupt_enable();
        return -1;
    }

    if (lock_holder[lock] == nullptr ||
        lock_holder[lock] != running_thread_ptr) {
        interrupt_enable();
        return -1;
    }

    lock_holder[lock] = nullptr;

    if (!lock_queue[lock].empty()) {
        thread_control_block* thread_ptr = lock_queue[lock].front();
        lock_queue[lock].pop();
        lock_holder[lock] = thread_ptr;
        ready_queue.push(thread_ptr);
    }

    interrupt_enable();

    return 0;
}

int thread_wait(unsigned int lock, unsigned int cond) {
    thread_unlock(lock);

    interrupt_disable();

    if (!is_initialized) {
        interrupt_enable();
        return -1;
    }

    cond_queue[{lock, cond}].push(running_thread_ptr);

    swapcontext(running_thread_ptr->ucontext_ptr, main_thread_ptr->ucontext_ptr);

    interrupt_enable();

    thread_lock(lock);

    return 0;
}

int thread_signal(unsigned int lock, unsigned int cond) {
    interrupt_disable();

    if (!is_initialized) {
        interrupt_enable();
        return -1;
    }

    if (!cond_queue[{lock, cond}].empty()) {
        ready_queue.push(cond_queue[{lock, cond}].front());
        cond_queue[{lock, cond}].pop();
    }

    interrupt_enable();

    return 0;
}

int thread_broadcast(unsigned int lock, unsigned int cond) {
    interrupt_disable();

    if (!is_initialized) {
        interrupt_enable();
        return -1;
    }

    while (!cond_queue[{lock, cond}].empty()) {
        ready_queue.push(cond_queue[{lock, cond}].front());
        cond_queue[{lock, cond}].pop();
    }

    interrupt_enable();

    return 0;
}