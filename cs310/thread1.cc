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

// the queue needs to be handled
static queue<Thread*> readyQueue;
// empty Context pointer for temp store.
static ucontext_t* globalContext = NULL;
// Current Running Thread
static Thread* currentThread = NULL;

// 因为方法里面的参数是unsigned int, 所以用一个map来存, 
static map<unsigned int, queue<Thread*> > conditions;
static map<unsigned int, Lock*> lockMap;

// 是否已经初始化的标志位
static bool libInitialized = false;

// type define thread_startfunc_t
typedef void (*thread_startfunc_t)(void *);

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
	// if the lib has already been initialized, return -1;
	if (libInitialized) {
		return -1;
	}

	libInitialized = true;
    interrupt_disable();
	// whether the thread has been created
	if (thread_create(func, arg) == -1) {
		return -1;
	}

	if (readyQueue.empty()) {
		return -1;
	}

	globalContext = new ucontext_t;
	getcontext(globalContext);
    //****switch
	currentThread = readyQueue.front();
    readyQueue.pop();
    //******

	swapcontext(globalContext, currentThread->_Context);
    //cout<<"In init ,After swap context!"<<endl;
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
            //cout<<"current0 completed"<<endl;
		}
		//***switch
        Thread* t = readyQueue.front();
        readyQueue.pop();
        currentThread = t;
        //***switch
        swapcontext(globalContext, currentThread->_Context);
        //cout<<"current1 completed"<<endl;
	}
    if(!currentThread)
    {
        //cout<<"current2 completed"<<endl;
        delete currentThread->stack;
        currentThread->_Context->uc_stack.ss_sp = NULL;
        currentThread->_Context->uc_stack.ss_size = 0;
        currentThread->_Context->uc_stack.ss_flags = 0;
        currentThread->_Context->uc_link = NULL;
        delete currentThread->_Context;
        delete currentThread;
        currentThread = NULL;
        //cout<<"current3 completed"<<endl;
    }
	// end here
	cout << "Thread library exiting.\n";
    exit(0);
}

// Step2
// When a thread calls thread_create, the caller does not yield the CPU. 
// The newly created thread is put on the ready queue but is not executed right away.
int thread_create(thread_startfunc_t func, void *arg) {

	if (!libInitialized) {
		return -1;
	}

	interrupt_disable();

	static int threadId = 0;


	// 创建对象
    try
    {
	Thread* thread;
	thread = new Thread;
	thread->_Context = new ucontext_t;
	thread->thread_ID = threadId;
	threadId++;
    getcontext(thread->_Context);
    // to stack
	thread->stack = new char[STACK_SIZE];
	thread->ifCompleted = false;
	thread->_Context->uc_stack.ss_sp = thread->stack;
	thread->_Context->uc_stack.ss_size = STACK_SIZE;
	thread->_Context->uc_stack.ss_flags = 0;
	thread->_Context->uc_link = NULL;
    /*
     * Direct the new thread to start by calling start(arg1, arg2).
     */
    // 需要一个start函数开始
    makecontext(thread->_Context, (void (*)()) start, 2, func, arg);

    readyQueue.push(thread);
    }

    catch (bad_alloc) {
        interrupt_enable();
        return -1;
    }
    interrupt_enable();
	return 0;
}

// Step1
int thread_yield(void) {
	if (!libInitialized) {
		return -1;
	}
	// Lecture10 Page29
	interrupt_disable();

	// 当暂停的时候, 挂起当前线程, 转换上下文
	// 用GlobalContext这个指针存
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