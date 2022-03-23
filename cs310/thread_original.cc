#include <cstdlib>
#include <ucontext.h>
#include <queue>
#include <map>
#include <iostream>

#include "interrupt.h"
#include "thread.h"

using namespace std;

struct TCB {
  ucontext_t* ucontext;
  int status;
};

static bool islib = false;
static TCB* RUNNING_THREAD;
static TCB* SWITCH_THREAD;

static queue<TCB*> READY_QUEUE;
static map<unsigned int, TCB*> LOCK_OWNER_MAP;
static map<unsigned int, queue<TCB*>> LOCK_QUEUE_MAP;
static map<pair<unsigned int, unsigned int>, queue<TCB*>> CV_QUEUE_MAP;

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

static void process(thread_startfunc_t func, void *arg) {
    while (!READY_QUEUE.empty()) {
        cleanup();
        RUNNING_THREAD = READY_QUEUE.front();
        READY_QUEUE.pop();
        swapcontext(SWITCH_THREAD->ucontext, RUNNING_THREAD->ucontext);
    }

    cleanup();

    cout << "Thread library exiting." << endl;

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
    makecontext(SWITCH_THREAD->ucontext, (void (*)()) process, 2, func, arg);

    if (thread_create(func, arg) == -1) {
        return -1;
    }

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

    TCB* newThread;

    newThread = new TCB;
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

    if (LOCK_QUEUE_MAP.count(lock) == 0) {
        LOCK_OWNER_MAP[lock] = NULL;
        LOCK_QUEUE_MAP.insert(pair<unsigned int, queue<TCB*>>(lock, queue<TCB*>()));
    }

    if (LOCK_OWNER_MAP[lock] != NULL) {
        LOCK_QUEUE_MAP[lock].push(RUNNING_THREAD);
        swapcontext(RUNNING_THREAD->ucontext, SWITCH_THREAD->ucontext);
    }
    else {
        LOCK_OWNER_MAP[lock] = RUNNING_THREAD;
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

    if (CV_QUEUE_MAP.find({lock, cond}) == CV_QUEUE_MAP.end()){
        queue<TCB*> NEW_CV_QUEUE;
        CV_QUEUE_MAP[{lock, cond}] = NEW_CV_QUEUE;
    }

    CV_QUEUE_MAP[{lock, cond}].push(RUNNING_THREAD);

    swapcontext(RUNNING_THREAD->ucontext, SWITCH_THREAD->ucontext);

    interrupt_enable();

    return thread_lock(lock);
}

int thread_signal(unsigned int lock, unsigned int cond) {
    interrupt_disable();

    if (!islib) {
        interrupt_enable();
        return -1;
    }

    if (!CV_QUEUE_MAP[{lock, cond}].empty()) {
        READY_QUEUE.push(CV_QUEUE_MAP[{lock, cond}].front());
        CV_QUEUE_MAP[{lock, cond}].pop();
    }

    interrupt_enable();

    return 0;
}

int thread_broadcast(unsigned int lock, unsigned int cond) {
    interrupt_disable();

    if (!islib) {
        interrupt_enable();
        return -1;
    }

    while (!CV_QUEUE_MAP[{lock, cond}].empty()) {
        READY_QUEUE.push(CV_QUEUE_MAP[{lock, cond}].front());
        CV_QUEUE_MAP[{lock, cond}].pop();
    }

    interrupt_enable();

    return 0;
}