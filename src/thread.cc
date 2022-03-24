#include <iostream>
#include <queue>
#include <map>
#include <ucontext.h>

#include "thread.h"
#include "interrupt.h"

using namespace std;

struct thread_control_block {
    bool is_finished;
    ucontext_t* ucontext_ptr;
    thread_control_block() : is_finished(false), ucontext_ptr(nullptr) {}
};

static bool is_initialized = false;
static thread_control_block* running_thread_ptr;
static thread_control_block* main_thread_ptr;

static queue<thread_control_block*> ready_queue;
static map<unsigned int, thread_control_block*> lock_holder;
static map<unsigned int, queue<thread_control_block*>> lock_queue;
static map<unsigned int, queue<thread_control_block*>> cond_queue;

static void release() {
    if (!running_thread_ptr || !running_thread_ptr->is_finished) {
        return;
    }

    delete (char*) running_thread_ptr->ucontext_ptr->uc_stack.ss_sp;
    running_thread_ptr->ucontext_ptr->uc_stack.ss_sp    = nullptr;
    running_thread_ptr->ucontext_ptr->uc_stack.ss_size  = 0;
    running_thread_ptr->ucontext_ptr->uc_stack.ss_flags = 0;
    running_thread_ptr->ucontext_ptr->uc_link           = nullptr;
    delete running_thread_ptr->ucontext_ptr;
    delete running_thread_ptr;
    running_thread_ptr = nullptr;
}

static void scheduler() {
    while (!ready_queue.empty()) {
        release();
        running_thread_ptr = ready_queue.front();
        ready_queue.pop();
        swapcontext(main_thread_ptr->ucontext_ptr, running_thread_ptr->ucontext_ptr);
    }

    release();

    cout << "Thread library exiting." << endl;

    exit(0);
}

static void thread_monitor(thread_startfunc_t func, void* arg) {
    interrupt_enable();
    func(arg);
    interrupt_disable();
    running_thread_ptr->is_finished = true;
    swapcontext(running_thread_ptr->ucontext_ptr, main_thread_ptr->ucontext_ptr);
}

static int my_unlock(unsigned int lock) {
    if (lock_holder[lock] == nullptr || lock_holder[lock] != running_thread_ptr) {
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

    return 0;
}

int thread_libinit(thread_startfunc_t func, void* arg) {
    if (is_initialized) {
        return -1;
    }

    is_initialized = true;
    main_thread_ptr = new thread_control_block();
    main_thread_ptr->is_finished = false;
    main_thread_ptr->ucontext_ptr = new ucontext_t;
    getcontext(main_thread_ptr->ucontext_ptr);
    main_thread_ptr->ucontext_ptr->uc_stack.ss_sp    = new char[STACK_SIZE];
    main_thread_ptr->ucontext_ptr->uc_stack.ss_size  = STACK_SIZE;
    main_thread_ptr->ucontext_ptr->uc_stack.ss_flags = 0;
    main_thread_ptr->ucontext_ptr->uc_link           = nullptr;
    makecontext(main_thread_ptr->ucontext_ptr, (void (*)()) scheduler, 0);

    int ret = thread_create(func, arg);
    if (ret == -1) {
        return -1;
    }

    running_thread_ptr = ready_queue.front();
    ready_queue.pop();

    func(arg);

    interrupt_disable();

    running_thread_ptr->is_finished = true;

    swapcontext(running_thread_ptr->ucontext_ptr, main_thread_ptr->ucontext_ptr);

    return 0;
}

int thread_create(thread_startfunc_t func, void* arg) {
    if (!is_initialized) {
        return -1;
    }

    interrupt_disable();

    thread_control_block* new_thread            = new thread_control_block();
    new_thread->ucontext_ptr                    = new ucontext_t;
    getcontext(new_thread->ucontext_ptr);
    new_thread->ucontext_ptr->uc_stack.ss_sp    = new char[STACK_SIZE];
    new_thread->ucontext_ptr->uc_stack.ss_size  = STACK_SIZE;
    new_thread->ucontext_ptr->uc_stack.ss_flags = 0;
    new_thread->ucontext_ptr->uc_link           = nullptr;
    makecontext(new_thread->ucontext_ptr, (void (*)())thread_monitor, 2, func, arg);

    ready_queue.push(new_thread);

    interrupt_enable();

    return 0;
}

int thread_yield(void) {
    if (!is_initialized) {
        return -1;
    }

    interrupt_disable();

    ready_queue.push(running_thread_ptr);

    swapcontext(running_thread_ptr->ucontext_ptr, main_thread_ptr->ucontext_ptr);

    interrupt_enable();

    return 0;
}

int thread_lock(unsigned int lock) {
    if (!is_initialized) {
        return -1;
    }

    interrupt_disable();

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
    if (!is_initialized) {
        return -1;
    }

    interrupt_disable();

    int ret = my_unlock(lock);

    interrupt_enable();

    return ret;
}

int thread_wait(unsigned int lock, unsigned int cond) {
    if (!is_initialized) {
        return -1;
    }

    interrupt_disable();

    if (my_unlock(lock) == -1) {
        return -1;
    }

    cond_queue[cond].push(running_thread_ptr);

    swapcontext(running_thread_ptr->ucontext_ptr, main_thread_ptr->ucontext_ptr);

    interrupt_enable();

    return thread_lock(lock);
}

int thread_signal(unsigned int lock, unsigned int cond) {
    if (!is_initialized) {
        return -1;
    }

    interrupt_disable();

    if (!cond_queue[cond].empty()) {
        ready_queue.push(cond_queue[cond].front());
        cond_queue[cond].pop();
    }

    interrupt_enable();

    return 0;
}

int thread_broadcast(unsigned int lock, unsigned int cond) {
    if (!is_initialized) {
        return -1;
    }

    interrupt_disable();

    while (!cond_queue[cond].empty()) {
        ready_queue.push(cond_queue[cond].front());
        cond_queue[cond].pop();
    }

    interrupt_enable();

    return 0;
}