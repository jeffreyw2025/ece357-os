#define _POSIX_SOURCE

# include "fifo.h"
# include "spinlock.h"
# include <stdio.h>
# include <unistd.h>
# include <sys/signal.h>
# include <sys/types.h>
# include <sys/wait.h>

void handler(int s){
    switch(s){
        default:
            break;
    }
}

void fifo_init(struct fifo *f){
    f->nextWrite = 0;
    f->nextRead = 0;
    f->itemCount = 0;
    f->sl.locked = 0;
    f->waiters = 0;
}

void fifo_wr(struct fifo *f,unsigned long d){
    // Obtain spinlock mutex to test condition- At this point we should be the only process holding the spinlock
    spin_lock(&(f->sl));
    // Check if FIFO full
    while(f->itemCount >= MYFIFO_BUFSIZ){
        sigset_t oldmask, newmask;
        signal(SIGUSR1, &handler);
        sigfillset(&newmask);
        sigprocmask(SIG_BLOCK,&newmask,&oldmask);
        f->waitlist[f->waiters++] = getpid();
        // Deadlock avoidance
        spin_unlock(&(f->sl));
        sigsuspend(&oldmask);
        // Reacquire spinlock before testing condition
        spin_lock(&(f->sl));
        sigprocmask(SIG_SETMASK,&oldmask,NULL);
    }
    // Critical Region Operations
    f->buf[f->nextWrite] = d;
    f->nextWrite = (f->nextWrite + 1) % MYFIFO_BUFSIZ;
    f->itemCount++;
    // Send a signal that fifo is no longer empty
    for(int i = 0; i < f->waiters; i++){
        kill(f->waitlist[i], SIGUSR1);
        f->waitlist[i] = 0;
    }
    f->waiters = 0;
    spin_unlock(&(f->sl));
}

unsigned long fifo_rd(struct fifo *f){
    unsigned long l;
    // Obtain spinlock mutex to test condition- At this point we should be the only process holding the spinlock
    spin_lock(&(f->sl));
    // Check if FIFO empty
    while(f->itemCount <= 0){
        sigset_t oldmask, newmask;
        signal(SIGUSR1, &handler);
        sigfillset(&newmask);
        sigprocmask(SIG_BLOCK,&newmask,&oldmask);
        f->waitlist[f->waiters++] = getpid();
        // Deadlock avoidance
        spin_unlock(&(f->sl));
        sigsuspend(&oldmask);
        // Reacquire spinlock before testing condition
        spin_lock(&(f->sl));
        sigprocmask(SIG_SETMASK,&oldmask,NULL);
    }
    // Critical Region Operations
    l = f->buf[f->nextRead];
    f->nextRead = (f->nextRead + 1) % MYFIFO_BUFSIZ;
    f->itemCount--;
    // Send a signal that fifo is no longer full
    for(int i = 0; i < f->waiters; i++){
        kill(f->waitlist[i], SIGUSR1);
        f->waitlist[i] = 0;
    }
    f->waiters = 0;
    spin_unlock(&(f->sl));
    return l;
}