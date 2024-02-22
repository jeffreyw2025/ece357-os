# include "spinlock.h"
# include <tas.h>
# include <unistd.h>

void spin_lock(struct spinlock *l){
    while(tas(&(l->locked)) != 0){};
    l->currentPID = getpid();
}

void spin_unlock(struct spinlock *l){
    l->currentPID = 0;
    l->locked = 0;
}