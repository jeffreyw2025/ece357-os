#ifndef _FIFO_H
#define _FIFO_H

#define MYFIFO_BUFSIZ 1024
#define NPROC 64

#include "spinlock.h"

volatile struct fifo{
    unsigned long buf[MYFIFO_BUFSIZ];
    int nextRead, nextWrite;
    int itemCount;
    // Technically there might be a problem if we have processes that are waiting on multiple
    // condition variables at the same time. In this case, however, our only conditions are
    // full and empty, and a FIFO can only be full and empty if it has size 0. So we are fine!
    int waitlist[NPROC];
    int waiters;
    struct spinlock sl; // Spinlock for implementing mutex
};

void fifo_init(struct fifo *f);
/* Initialize the shared memory FIFO *f including any required underlying
* initializations (such as calling cv_init). The FIFO will have a static
* fifo length of MYFIFO_BUFSIZ elements. #define this in fifo.h.
* A value of 1K is reasonable. In most cases, simply setting the
* entire struct fifo to 0 will suffice as initialization.
*/

void fifo_wr(struct fifo *f,unsigned long d);
/* Enqueue the data word d into the FIFO, blocking unless and until the
* FIFO has room to accept it. (i.e. block until !full)
* Wake up a reader which was waiting for the FIFO to be non-empty
*/

unsigned long fifo_rd(struct fifo *f);
/* Dequeue the next data word from the FIFO and return it. Block unless
* and until there are available words. (i.e. block until !empty)
* Wake up a writer which was waiting for the FIFO to be non-full
*/


#endif