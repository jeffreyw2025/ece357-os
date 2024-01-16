# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <errno.h>
# include <fcntl.h>
# include <unistd.h>
# include <linux/limits.h>
# include <sys/types.h>
# include <sys/wait.h>
# include <sys/signal.h>

// launcher.c
// By: Jeffrey Wong
/* This program executes wordgen, wordsearch, and pager, and establishes
two pipes from wordgen to wordsearch and from wordsearch to pager. The program
accepts a single argument which is the number of words to be generated by wordgen */

int main(int argc, char **argv){
    // Capture argument for wordgen
    long numwords = 0, candidate, wordsgenerated = 0;
    if(argc > 1){
        candidate = strtol(argv[1], NULL, 10);
        if(errno){
            fprintf(stderr, "Error: Argument to wordgen invalid: %s", strerror(errno));
            return -1;
        }
        numwords = candidate > 0 ? candidate : 0;
    }

    // Set up pipes for processes- Structure based on code at https://stackoverflow.com/questions/32839904/piping-between-processes-in-c
    int genToSearchfds[2];
    int searchToPagerfds[2];
    if(pipe(genToSearchfds) < 0){
        fprintf(stderr, "Cannot create pipe between wordgen and wordsearch: %s", strerror(errno));
        return -1;
    }
    if(pipe(searchToPagerfds) < 0){
        fprintf(stderr, "Cannot create pipe between wordsearch and pager: %s", strerror(errno));
        return -1;
    }
    int wordgenpid = fork();
    switch(wordgenpid){
        case -1:
            fprintf(stderr, "Error occured while attempting to fork wordgen: %s\n", strerror(errno));
            return -1;
        case 0:
            // Close unnecessary fds and redirect std i/o
            close(searchToPagerfds[0]); // 0 represents the read side, 1 represents the write side
            close(searchToPagerfds[1]);
            close(genToSearchfds[0]);
            dup2(genToSearchfds[1],1);
            close(genToSearchfds[1]);
            char *wordcount;
            sprintf(wordcount, "%ld", numwords);
            if(execlp("./wordgen", "./wordgen", wordcount, (char *)NULL) < 0){
                fprintf(stderr,"Error occured while attempting to exec wordgen: %s\n", strerror(errno));
                exit(127);
            }
            break;
        default:
            close(genToSearchfds[1]);
            break;
    }

    int wordsearchpid = fork();
    switch(wordsearchpid){
        case -1:
            fprintf(stderr, "Error occured while attempting to fork wordsearch: %s\n", strerror(errno));
            return -1;
        case 0:
            // Close unnecessary fds and redirect std i/o
            close(searchToPagerfds[0]); 
            dup2(searchToPagerfds[1],1);
            close(searchToPagerfds[1]);
            dup2(genToSearchfds[0],0);
            close(genToSearchfds[0]);
            if(execlp("./wordsearch", "./wordsearch", "words.txt", (char *)NULL)<0){
                fprintf(stderr,"Error occured while attempting to exec wordsearch: %s\n", strerror(errno));
                exit(127);
            }
            break;
        default:
            close(searchToPagerfds[1]);
            close(genToSearchfds[0]);
            break;
    }

    int pagerpid = fork();
    switch(pagerpid){
        case -1:
            fprintf(stderr, "Error occured while attempting to fork pager: %s\n", strerror(errno));
            return -1;
        case 0:
            // Close unnecessary fds and redirect std i/o
            dup2(searchToPagerfds[0],0);
            close(searchToPagerfds[0]);  
            if(execlp("./pager", "./pager", (char *)NULL)<0){
                fprintf(stderr,"Error occured while attempting to exec pager: %s\n", strerror(errno));
                exit(127);
            }
            break;
        default:
            close(searchToPagerfds[0]); 
            break;
    }
    unsigned status;
    int cpid;
    while((cpid = wait(&status)) > 0){
        printf("Child %d exited with %d\n", cpid, status);
    }
    if(errno != 0 && errno != ECHILD){ // An ECHILD is fine, that means all our child processes have exited.
        fprintf(stderr, "Error while attempting to wait for child: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}