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
# include "fifo.h"
# include <tas.h>

int main(int argc, char** argv){
    // Preliminary error handling
    if(argc < 3){
        fprintf(stderr, "Error: Insufficient arguments passed to ftest\n");
        return -1;
    }
    unsigned long numWriters = strtol(argv[1], NULL, 10);
    if(errno){
        fprintf(stderr, "Error: Argument to ftest for number of writers invalid: %s\n", strerror(errno));
        return -1;
    }
    if(numWriters > NPROC){
        fprintf(stderr, "Warning: Too many writers specified: Capping writers to %d\n", NPROC);
        numWriters = (long)NPROC;
    }
    unsigned long numIter = strtol(argv[2], NULL, 10);
    if(errno){
        fprintf(stderr, "Error: Argument to ftest for number of iterations invalid: %s\n", strerror(errno));
        return -1;
    }
    if(numIter > 0xFFFFFF){
        fprintf(stderr, "Warning: Too many iterations specified: Capping writers to 0xFFFFFF = 16777215\n");
        numIter = 0xFFFFFF;
    }

    fprintf(stderr, "Beginning test with %ld writers and %ld items each\n", numWriters, numIter);
    
    fprintf(stderr, "Attempting to create FIFO\n");
    struct fifo *f;
    if((f = (struct fifo *)mmap(NULL, sizeof(struct fifo), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0)) < 0){
        fprintf(stderr, "Error attempting to map shared memory region for FIFO: %s\n", strerror(errno));
        return -1;
    }

    fprintf(stderr, "Initializing FIFO\n");
    fifo_init(f);
    
    for(unsigned long i = 0; i < numWriters; i++){
        int myProcNum = i;
        int writerpid;
        switch(writerpid = fork()){
            case -1:
                fprintf(stderr, "Error occured while attempting to fork to writer child: %s\n", strerror(errno));
                return -1;
            case 0:
                for(unsigned long j = 0; j < numIter; j++){
                    // The lower 24 bits comprise the sequence number, while the upper bits will denote the writer number up to 63
                    unsigned long word = (myProcNum << 24) + j;
                    fifo_wr(f, word);
                }
                fprintf(stderr, "Writer stream %ld completed\n", i);
                exit(0);
                break;
            default:
                break;
        }
    }   

    long currentExpectedValues[numWriters]; // Array representing what the next value we should read from a given writer process should be
    for(long i = 0; i < numWriters; i++){
        currentExpectedValues[i] = 0;
    }
    
    int readerpid;
    switch(readerpid = fork()){
        case -1:
            fprintf(stderr, "Error occured while attempting to fork to reader child: %s\n", strerror(errno));
            break;
        case 0:
            for(long i = 0; i < numWriters*numIter; i++){
                unsigned long nextData = fifo_rd(f);
                unsigned long writerID = (nextData >> 24);
                if(currentExpectedValues[writerID] >= 0){
                    if((nextData & 0xFFFFFF) != currentExpectedValues[writerID]){
                        fprintf(stderr, "Data received out of sequence from writer %ld\n", writerID);
                        currentExpectedValues[writerID] = -1; // Flag that we have received out of sequence data from that writer
                    }
                    else if(++currentExpectedValues[writerID] == numIter){
                        fprintf(stderr, "Reader stream %ld completed successfully\n", writerID);
                    }
                }
            }
            fprintf(stderr, "Recap:\n");
            int failedStreams = 0;
            for(long i = 0; i < numWriters; i++){
                if(currentExpectedValues[i] == numIter){
                    fprintf(stderr, "Reader stream %ld: Pass\n", i);
                }
                else if(currentExpectedValues[i] < 0){
                    fprintf(stderr, "Reader stream %ld: Fail (Out of order)\n", i);
                    failedStreams++;
                }
                else{
                    fprintf(stderr, "Reader stream %ld: Fail (Wrong number of values)\n", i);
                    failedStreams++;
                }
            }
            if(failedStreams){
                fprintf(stderr, "Number of failed streams: %d\n", failedStreams);
            }
            else{
                fprintf(stderr, "All reader streams pass!!!\n");
            }
        default:
            errno = 0;
            int status;
            while((status = wait(NULL)) > 0){;} // This stalls the parent function until there are no more "spammer" children are left
            // Found at https://stackoverflow.com/questions/19461744/how-to-make-parent-wait-for-all-child-processes-to-finish
            if(status == 0 || errno == ECHILD){
                fprintf(stderr, "All children finished sending\n");
            }
            else{
                fprintf(stderr, "Error while waiting for children: %s\n", strerror(errno));
            }
            break;
    }
    return 0;
}