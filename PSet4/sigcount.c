# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <errno.h>
# include <unistd.h>
# include <sys/signal.h>
# include <sys/types.h>
# include <sys/wait.h>

int handler(int s){
    static int sigcount;
    switch(s){
        case SIGUSR1: case 42: // SIGUSR1 is our "standard" test signal, 42 is the answer to life, the universe, and everything, and is our real-time test signal
            sigcount++;
            break;
        case SIGUSR2:
            fprintf(stderr, "Signal %s received %d times.\n", strsignal(SIGUSR1), sigcount);
            exit(0);
        default:
            break;
    }    
}

int sendsignals(int pid, int sigcount, int iter){
    int senderpid;
    switch(senderpid = fork()){
        case -1:
            fprintf(stderr, "Error occured while attempting to fork to \"sender\" child: %s\n", strerror(errno));
            return -1;
        case 0: // Sender Child Process
            for(int i = 0; i < iter; i++){
                kill(pid, sigcount);
            }
            exit(0);
            break;
        default: // Parent Process
            break;
    }
}

int main(int argc, char* argv[]){
    int pid;
    int numchilds = 100, sigcount = 100;
    int spamsig = 42, termsig = SIGUSR2;
    switch(pid = fork()){
        case -1:
            fprintf(stderr, "Error occured while attempting to fork to \"victim\" child: %s\n", strerror(errno));
            return -1;
        case 0: // Victim Child Process
            if(signal(spamsig, &handler)==SIG_ERR){
                perror("Invalid signal number");
                exit(-1);
            }
            if(signal(termsig, &handler)==SIG_ERR){
                perror("Invalid signal number");
                exit(-1);
            }
            for(;;)
            break;
        default: // Parent Process
            for(int i = 0; i < numchilds; i++){
                sendsignals(pid, spamsig, sigcount);
            }
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
            fprintf(stderr, "Sending terminating signal %s\n", strsignal(termsig));
            fprintf(stderr, "Expected number: %d\n", numchilds * sigcount);
            kill(pid, termsig);
            break;
    }
    return 0;
}