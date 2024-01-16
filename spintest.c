# include <stdio.h>
# include <stdlib.h>
# include <fcntl.h>
# include <string.h>
# include <errno.h>
# include <unistd.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <sys/mman.h>
# include <sys/wait.h>
# include "spinlock.h"
# include <tas.h>

// Spinlock Test Program
// By: Jeffrey Wong
/* This program uses a custom spinlock implementation to implement
mutex in a shared memory region. In this program we create a shared memory
region for a long and create multiple children to perform a series of 
non-atomic increments. The final count should match only if mutex is used. */

int main(int argc, char** argv){
    // Preliminary error handling
    if(argc < 3){
        fprintf(stderr, "Error: Insufficient arguments passed to spintest\n");
        return -1;
    }
    long numChildren = strtol(argv[1], NULL, 10);
    if(errno){
        fprintf(stderr, "Error: Argument to spintest for number of children invalid: %s", strerror(errno));
        return -1;
    }
    long numIter = strtol(argv[2], NULL, 10);
    if(errno){
        fprintf(stderr, "Error: Argument to spintest for number of iterations invalid: %s", strerror(errno));
        return -1;
    }
       
    long *count;

    if((count = (long *)mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0)) < 0){
        fprintf(stderr, "Error attempting to map shared memory region for count: %s\n", strerror(errno));
        return -1;
    }

    fprintf(stderr, "Attempting to create spinlock\n");
    struct spinlock *sl;
    if((sl = (struct spinlock *)mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0)) < 0){
        fprintf(stderr, "Error attempting to map shared memory region for spinlock: %s\n", strerror(errno));
        return -1;
    }
    sl->locked = 0;
    sl->currentPID = 0;
   
    *count = 0;

    fprintf(stderr, "Initializing incrementation procedure\n");

    for(long i = 0; i < numChildren; i++){
        int pid;
        switch(pid = fork()){
            case -1:
                fprintf(stderr, "Error occured while attempting to fork to child: %s\n", strerror(errno));
                return -1;
            case 0:
                for(long j = 0; j < numIter; j++){
                    spin_lock(sl);
                    // This is the critical region operation
                    // When we get here, we should be the only task holding the spinlock sl
                    (*count)++;
                    spin_unlock(sl);
                }
                exit(0);
                break;
            default:
                break;
        }
    }
    errno = 0;
    int status;
    while((status = wait(NULL)) > 0){;} // Same wait code as in sigcount program
    if(status == 0 || errno == ECHILD){
        fprintf(stderr, "All children finished incrementing\n");
    }
    else{
        fprintf(stderr, "Error while waiting for children: %s\n", strerror(errno));
    }
    fprintf(stderr, "Number of increments / expected number: %ld / %ld\n", *count, numChildren * numIter);   
    return 0;
}