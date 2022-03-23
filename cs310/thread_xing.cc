#include <cstdlib>
#include <iostream>
#include <map>
#include <queue>
#include <iterator>
#include <vector>
#include <ucontext.h>

#include "thread.h"
#include "interrupt.h"

using namespace std;

/* 
 * @Description
 * 按照线程的结构设计
 * 一个栈帧, 一个ID, 一个上下文的内容
 * 一个标志位boolean表示是否执行完毕
 * 用指针去存就不会有线性表的问题
 */
struct Thread
{
	unsigned int thread_ID;
	char* stack;
	bool ifCompleted;
	ucontext_t* _Context;
};

/*
 * @Description
 * 锁结构, 里面放一个等待队列, 不用全局的
 * 还有锁内部的线程, 同样放指针而不是对象
 */
struct Lock
{
	Thread* held;
	queue<Thread*> waitQueue;
};

static bool libInitialized = false;
static ucontext_t* globalContext = NULL;
static Thread* currentThread = NULL;

static queue<Thread*> readyQueue;
static map<unsigned int, queue<Thread*>> conditions;
static map<unsigned int, Lock*> lockMap;

static int start(thread_startfunc_t func, void *arg) {
    interrupt_enable();
    func(arg);
    interrupt_disable();
	currentThread->ifCompleted = true;
	swapcontext(currentThread->_Context, globalContext);
	return 0;
}

static int unlockFunc(unsigned int lock) {
	map<unsigned int, Lock*>::iterator iter = lockMap.find(lock);

	if (iter == lockMap.end()) {
		return -1;
	}

	Lock* unlock = iter->second;

	if (unlock->held == NULL) {
		return -1;
	}

	if (unlock->held->thread_ID != currentThread->thread_ID) {
		return -1;
	}

	if(!unlock->waitQueue.empty())
    {
        unlock->held = unlock->waitQueue.front();
        unlock->waitQueue.pop();
        readyQueue.push(unlock->held);
    } else {
    	unlock->held = NULL;
    }

    return 0;

}

int thread_libinit(thread_startfunc_t func, void *arg) {
	if (libInitialized) {
		return -1;
	}

	libInitialized = true;

	globalContext = new ucontext_t;
	getcontext(globalContext);

    thread_create(func, arg);
	currentThread = readyQueue.front();
    readyQueue.pop();

    interrupt_disable();

	swapcontext(globalContext, currentThread->_Context);

	while (!readyQueue.empty()) {
		if (currentThread->ifCompleted) {
            delete currentThread->stack;
            currentThread->_Context->uc_stack.ss_sp = NULL;
            currentThread->_Context->uc_stack.ss_size = 0;
            currentThread->_Context->uc_stack.ss_flags = 0;
            currentThread->_Context->uc_link = NULL;
            delete currentThread->_Context;
            delete currentThread;
            currentThread = NULL;
		}

        currentThread = readyQueue.front();
        readyQueue.pop();

        swapcontext(globalContext, currentThread->_Context);
	}

    if(!currentThread) {
        delete currentThread->stack;
        currentThread->_Context->uc_stack.ss_sp = NULL;
        currentThread->_Context->uc_stack.ss_size = 0;
        currentThread->_Context->uc_stack.ss_flags = 0;
        currentThread->_Context->uc_link = NULL;
        delete currentThread->_Context;
        delete currentThread;
        currentThread = NULL;
    }

	cout << "Thread library exiting.\n";

    exit(0);
}

int thread_create(thread_startfunc_t func, void *arg) {
	if (!libInitialized) {
		return -1;
	}

	interrupt_disable();

	static int threadId = 0;

    Thread* thread;
    thread = new Thread;
    thread->_Context = new ucontext_t;
    thread->thread_ID = threadId;
    threadId++;
    getcontext(thread->_Context);

    thread->stack = new char[STACK_SIZE];
    thread->ifCompleted = false;
    thread->_Context->uc_stack.ss_sp = thread->stack;
    thread->_Context->uc_stack.ss_size = STACK_SIZE;
    thread->_Context->uc_stack.ss_flags = 0;
    thread->_Context->uc_link = NULL;

    makecontext(thread->_Context, (void (*)()) start, 2, func, arg);

    readyQueue.push(thread);

    interrupt_enable();

	return 0;
}

int thread_yield(void) {
	if (!libInitialized) {
		return -1;
	}

	interrupt_disable();

	readyQueue.push(currentThread);

	swapcontext(currentThread->_Context, globalContext);

	interrupt_enable();

	return 0;
}

int thread_lock(unsigned int lock) {
	if (!libInitialized) {
		return -1;
	}

    interrupt_disable();

	map<unsigned int, Lock*>::iterator iter = lockMap.find(lock);

	if (iter == lockMap.end()) {
		Lock* mlock;
		try {
			mlock = new Lock;
		} catch(bad_alloc) {
			delete mlock;
			interrupt_enable();
			return -1;
		} 
		mlock->held = currentThread;
		lockMap[lock] = mlock;
	} else {
		Lock* pri_lock = iter->second;

		if (pri_lock->held == NULL) {
		// 如果没有则给当前线程
			pri_lock->held = currentThread;
		}
        else if (pri_lock->held->thread_ID == currentThread->thread_ID) {
		// 如果线程ID一样则返回
			interrupt_enable();
			return -1;
		} else {
		// 如果被占用则加入waitqueue
			pri_lock->waitQueue.push(currentThread);
			swapcontext(currentThread->_Context, globalContext);
		}
	}

    interrupt_enable();
    return 0;

}

int thread_unlock(unsigned int lock) {
	if (!libInitialized) {
		return -1;
	}

	interrupt_disable();

	int ifUnlock = unlockFunc(lock);

	interrupt_enable();

	return ifUnlock;
}

int thread_wait(unsigned int lock, unsigned int cond) {
	if(!libInitialized) {
        return -1;
    }

    interrupt_disable();

    int ifUnlock = unlockFunc(lock);
    //cout<<"ifunlock: "<<ifUnlock<<endl;
    if (ifUnlock == 0) {
    	conditions[cond].push(currentThread);
    	swapcontext(currentThread->_Context, globalContext);
    	interrupt_enable();
    	int doLock = thread_lock(lock);
    	return doLock;
    }

    interrupt_enable();
    return -1;
}

int thread_signal(unsigned int lock, unsigned int cond) {
	if (!libInitialized) {
		return -1;
	}

	interrupt_disable();

	// find all threads waiting for that cond
    map<unsigned int, queue<Thread*> >::iterator it = conditions.find(cond);

    if (it == conditions.end())
    {
        interrupt_enable();
        return 0;
    }
    else
    {
    // 如果都在等待, 选择最上面的线程放到readyQueue里面
        if(!it->second.empty())
        {
            Thread* t = it->second.front();
            it->second.pop();
            readyQueue.push(t);
        }
    }

	interrupt_enable();

	return 0;
}

int thread_broadcast(unsigned int lock, unsigned int cond) {
	if (!libInitialized) {
		return -1;
	}

	interrupt_disable();

	// find all threads waiting for that cond
    map<unsigned int, queue<Thread*> >::iterator it = conditions.find(cond);

    if (it == conditions.end()) {
        interrupt_enable();
        return 0;
    }
    // 如果都在等待, 选择上面的所有线程放到readyQueue里面
    else
    {
        while (!it->second.empty())
        {
            Thread *t = it->second.front();
            it->second.pop();
            readyQueue.push(t);
        }
    }

	interrupt_enable();

	return 0;
}