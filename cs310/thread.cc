#include <cstdlib>
#include <ucontext.h>
#include <queue>
#include <iterator>
#include <map>
#include <iostream>

#include "interrupt.h"
#include "thread.h"

using namespace std;

struct TCB {
    ucontext_t* ucontext;
    int status; // 0 for not finished, 3 for cleanup
};

static bool islib = false;
static TCB* SWITCH_THREAD;
static TCB* RUNNING_THREAD;

static queue<TCB*> READY_QUEUE;
static map<unsigned int, queue<TCB*>> LOCK_QUEUE_MAP;
static map<pair<unsigned int, unsigned int>, queue<TCB*>> CV_QUEUE_MAP;
static map<unsigned int, TCB*> LOCK_OWNER_MAP;

static void cleanup() {
    if (RUNNING_THREAD == NULL) {
        return;
    }

    if (RUNNING_THREAD->status == 3) {
        delete (char*) RUNNING_THREAD->ucontext->uc_stack.ss_sp;
        RUNNING_THREAD->ucontext->uc_stack.ss_sp = NULL;
        RUNNING_THREAD->ucontext->uc_stack.ss_size = 0;
        RUNNING_THREAD->ucontext->uc_stack.ss_flags = 0;
        RUNNING_THREAD->ucontext->uc_link = NULL;
        delete RUNNING_THREAD->ucontext;
        delete RUNNING_THREAD;
        RUNNING_THREAD = NULL;
    }
}

static void process() {
    while (!READY_QUEUE.empty()) {
        cleanup();
        RUNNING_THREAD = READY_QUEUE.front();
        READY_QUEUE.pop();
        swapcontext(SWITCH_THREAD->ucontext, RUNNING_THREAD->ucontext);
    }

    cleanup();

    cout << "Thread library exiting.\n";

    exit(0);
}

static void STUB(thread_startfunc_t func, void *arg) {
    interrupt_enable();
    func(arg);
    interrupt_disable();

    RUNNING_THREAD->status = 3;
    swapcontext(RUNNING_THREAD->ucontext, SWITCH_THREAD->ucontext);
}

int thread_libinit(thread_startfunc_t func, void *arg) {
    if (islib) {
        return -1;
    }

    islib = true;

    SWITCH_THREAD = new TCB;
    SWITCH_THREAD->status = 0;

    SWITCH_THREAD->ucontext = new ucontext_t;
    getcontext(SWITCH_THREAD->ucontext);
    SWITCH_THREAD->ucontext->uc_stack.ss_sp = new char [STACK_SIZE];
    SWITCH_THREAD->ucontext->uc_stack.ss_size = STACK_SIZE;
    SWITCH_THREAD->ucontext->uc_stack.ss_flags = 0;
    SWITCH_THREAD->ucontext->uc_link = NULL;
    makecontext(SWITCH_THREAD->ucontext, process, 0);

    thread_create(func, arg);

    RUNNING_THREAD = READY_QUEUE.front();
    READY_QUEUE.pop();

    func(arg);

    interrupt_disable();

    RUNNING_THREAD->status = 3;

    swapcontext(RUNNING_THREAD->ucontext, SWITCH_THREAD->ucontext);
}

int thread_create(thread_startfunc_t func, void *arg) {
    interrupt_disable();

    if (!islib) {
        interrupt_enable();
        return -1;
    }

    TCB* newThread = new TCB;
    newThread->status = 0;

    newThread->ucontext = new ucontext_t;
    getcontext(newThread->ucontext);
    newThread->ucontext->uc_stack.ss_sp = new char [STACK_SIZE];
    newThread->ucontext->uc_stack.ss_size = STACK_SIZE;
    newThread->ucontext->uc_stack.ss_flags = 0;
    newThread->ucontext->uc_link = NULL;
    makecontext(newThread->ucontext, (void (*)())STUB, 2, func, arg);

    READY_QUEUE.push(newThread);

    interrupt_enable();

    return 0;
}

int thread_yield(void) {
    interrupt_disable();

    if (!islib) {
        interrupt_enable();
        return -1;
    }

    READY_QUEUE.push(RUNNING_THREAD);

    swapcontext(RUNNING_THREAD->ucontext, SWITCH_THREAD->ucontext);

    interrupt_enable();

    return 0;
}

int thread_lock(unsigned int lock) {
    interrupt_disable();

    if (!islib) {
        interrupt_enable();
        return -1;
    }

    if (LOCK_OWNER_MAP[lock] == RUNNING_THREAD) {
        interrupt_enable();
        return -1;
    }

    // Check if there is a queue for the lock in the
    // lock queue map, and if not -- add one.
    if (LOCK_QUEUE_MAP.count(lock) == 0) {
        LOCK_OWNER_MAP[lock] = NULL; // Lock has no owner yet.
        queue<TCB*> NEW_LOCK_QUEUE; // Create empty queue.
        LOCK_QUEUE_MAP.insert(pair<unsigned int, queue<TCB*> >(lock, NEW_LOCK_QUEUE) ); // Insert.
    }

    // If the lock is owned by another thread.
    if (LOCK_OWNER_MAP[lock] != NULL) {
        LOCK_QUEUE_MAP[lock].push(RUNNING_THREAD); // Push current thread to end of ready queue.
        // Switch thread to run the head of the ready queue.
        swapcontext(RUNNING_THREAD->ucontext, SWITCH_THREAD->ucontext);
    }
    else {
        LOCK_OWNER_MAP[lock] = RUNNING_THREAD; // Give lock to this thread.
    }

    interrupt_enable();

    return 0;
}

int thread_unlock(unsigned int lock) {
    interrupt_disable();

    if (!islib) {
        interrupt_enable();
        return -1;
    }

    if (LOCK_OWNER_MAP.count(lock) == 0 ||
        LOCK_OWNER_MAP[lock] == NULL ||
        LOCK_OWNER_MAP[lock] != RUNNING_THREAD) {
        interrupt_enable();
        return -1;
    }

    LOCK_OWNER_MAP[lock] = NULL;

    if (!LOCK_QUEUE_MAP[lock].empty()) {
        READY_QUEUE.push(LOCK_QUEUE_MAP[lock].front());
        LOCK_OWNER_MAP[lock] = LOCK_QUEUE_MAP[lock].front();
        LOCK_QUEUE_MAP[lock].pop();
    }

    interrupt_enable();

    return 0;
}

int thread_wait(unsigned int lock, unsigned int cond) {
    interrupt_disable();

    if (!islib) {
        interrupt_enable();
        return -1;
    }

    if (LOCK_OWNER_MAP.count(lock) == 0 ||
        LOCK_OWNER_MAP[lock] == NULL ||
        LOCK_OWNER_MAP[lock] != RUNNING_THREAD) {
        interrupt_enable();
        return -1;
    }

    LOCK_OWNER_MAP[lock] = NULL;

    if (!LOCK_QUEUE_MAP[lock].empty()) {
        READY_QUEUE.push(LOCK_QUEUE_MAP[lock].front());
        LOCK_OWNER_MAP[lock] = LOCK_QUEUE_MAP[lock].front();
        LOCK_QUEUE_MAP[lock].pop();
    }

    CV_QUEUE_MAP[{lock, cond}].push(RUNNING_THREAD);

    swapcontext(RUNNING_THREAD->ucontext, SWITCH_THREAD->ucontext);

    interrupt_enable();

    thread_lock(lock);

    return 0;
}

int thread_signal(unsigned int lock, unsigned int cond){
    interrupt_disable();

    if (!islib) {
        interrupt_enable();
        return -1;
    }

    if (!CV_QUEUE_MAP[{lock,cond}].empty()) {
        READY_QUEUE.push(CV_QUEUE_MAP[{lock,cond}].front());
        CV_QUEUE_MAP[{lock,cond}].pop();
    };

    interrupt_enable();

    return 0;
}

int thread_broadcast(unsigned int lock, unsigned int cond) {
    interrupt_disable();

    if (!islib) {
        interrupt_enable();
        return -1;
    }

    pair<unsigned int, unsigned int> lock_cond_pair = make_pair(lock,cond);
    while (!CV_QUEUE_MAP[lock_cond_pair].empty()){
        READY_QUEUE.push(CV_QUEUE_MAP[lock_cond_pair].front());
        CV_QUEUE_MAP[lock_cond_pair].pop();
    }

    interrupt_enable();

    return 0;
}