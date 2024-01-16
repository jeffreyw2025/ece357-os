#ifndef _SPINLOCK_H
#define _SPINLOCK_H

struct spinlock{
    volatile char locked;
    int currentPID;
};

void spin_lock(struct spinlock *l);

void spin_unlock(struct spinlock *l);

#endif