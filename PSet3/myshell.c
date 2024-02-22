# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <errno.h>
# include <fcntl.h>
# include <unistd.h>
# include <linux/limits.h>
# include <sys/types.h>
# include <sys/wait.h>
# include <sys/resource.h>
# include <time.h>
# include <sys/time.h>
# include <sys/signal.h>

// myshell.c
// By: Jeffrey Wong
/* This program mimics some of the basic functionality of the UNIX shell.
It accepts commands from a parent shell or shell scripts then executes line by line.
I/O redirection is supported, as are the built-in commands cd, pwd, and exit.  */

// Gets a null-terminated substring of length len starting from position pos from str
char* substring(char *str, int pos, int len){
    if(pos+len > strlen(str)){
        fprintf(stderr, "Error while calling substring: Length requested exceeds available characters.");
        return NULL;
    }
    char temp[4096];
    memcpy(temp,str+pos,len);
    temp[pos+len] = '\0';
    char *result = strdup(temp); // This implicitly mallocs, make sure to free later
    return result;
}

// Used to redirect standard input, output, and error to files in child.
int redirectStd(int oldfd, char *fname, int flags){
    int newfd;
    char *channel; // Mostly for printing an appropriate error
    switch(oldfd){
        case 0:
            channel = "standard input";
            break;
        case 1:
            channel = "standard output";
            break;
        case 2:
            channel = "standard error";
            break;
        default:
            fprintf(stderr, "Error: Tried to redirect non-standard file descriptor: %d\n", oldfd);
            return -1;
    }
    if((newfd = open(fname,flags,0666)) < 0){
        fprintf(stderr, "Error: Could not redirect %s to %s: %s\n", channel, fname, strerror(errno));
        return -1;
    }
    if(dup2(newfd,oldfd) < 0){
        fprintf(stderr, "Attempt to dup %s to %s failed: %s\n", fname, channel, strerror(errno)); // This should not happen normally with standard channels
        return -1;
    }
    close(newfd);
    return 0;
}

// Returns the temporal difference between two timespec structs pointers as a timespec struct pointer
struct timeval* timediff(struct timeval starttime, struct timeval endtime){
    time_t secs;
    long usecs;
    if(endtime.tv_usec < starttime.tv_usec){
        secs = endtime.tv_sec - starttime.tv_sec - 1;
        usecs = endtime.tv_usec - starttime.tv_usec + 1000000L;
    }
    else{
        secs = endtime.tv_sec - starttime.tv_sec;
        usecs = endtime.tv_usec - starttime.tv_usec;
    }
    struct timeval *result;
    result->tv_sec = secs;
    result->tv_usec = usecs;
    return result;
}

int main(int argc, char* argv[]){
    FILE *scriptfile; // By default we read from stdin, we only read from file if it is specified
    int lastexitstatus = 0;
    if(argc > 2){
        fprintf(stderr, "Too many arguments passed to myshell. Fallback to default behavior: read from terminal\n");
    }
    else if(argc == 2){
        if((scriptfile = fopen(argv[1], "r"))<0){
            fprintf(stderr, "Error attempting to open file %s for reading: %s\n", argv[1], strerror(errno));
            return -1;
        }
    }

    char cmdline[4096]; // Each command will process up to 4095 characters at once. If you need more, tough luck.
    while(1){
        // fgets returns NULL and breaks loop if EOF encountered or if there is an error
        if(argc == 2){
            if(!fgets(cmdline, 4096, scriptfile)){break;}
        }
        else if(!fgets(cmdline, 4096, stdin)){break;}

        if(strlen(cmdline) < 1){continue;} // Ignore empty lines
        if(cmdline[0]=='#'){continue;} // # denotes a comment, we ignore lines with comments
        // Track # arguments, whether standard channels should be redirected, and whether to truncate or append output or error
        int internalargc = 1, redir_in = 0, redir_out = 0, redir_err = 0, outflag, errflag;
        char *infile = (char *)NULL, *outfile = (char *)NULL, *errfile = (char *)NULL, *curarg; // Track names of files to redirect to as well as current argument for tokenization
        if((curarg = strtok(cmdline, " \n")) == NULL){continue;} // If our first strtok retuns null then the line has no tokens, ignore it
        char **internalargv = malloc(2*sizeof(infile)); // We will dynamically add arguments to this array during tokenization
        internalargv[0] = curarg;
        while((curarg = strtok(NULL, " \n")) != NULL){
            // Redirect standard input
            if(!strncmp(curarg,"<",1)){
                if(redir_in){
                    fprintf(stderr, "Warning: Cannot redirect stdin twice! Second redirection ignored.\n");
                    continue;
                }
                infile = substring(curarg,1,strlen(curarg)-1);
                redir_in = 1;
            }
            // Redirect standard output
            else if(!strncmp(curarg,">",1)){
                if(redir_out){
                    fprintf(stderr, "Warning: Cannot redirect stdout twice! Second redirection ignored.\n");
                    continue;
                }
                if(!strncmp(curarg,">>",2)){
                    outfile = substring(curarg,2,strlen(curarg)-2);
                    outflag = O_APPEND;
                }
                else{
                    outfile = substring(curarg,1,strlen(curarg)-1);
                    outflag = O_TRUNC;
                }
                redir_out = 1;
            }
            // Redirect standard error
            else if(!strncmp(curarg,"2>",2)){
                if(redir_err){
                    fprintf(stderr, "Warning: Cannot redirect stderr twice! Second redirection ignored.\n");
                    continue;
                }
                if(!strncmp(curarg,"2>>",3)){
                    errfile = substring(curarg,3,strlen(curarg)-3);
                    errflag = O_APPEND;
                }
                else{
                    errfile = substring(curarg,2,strlen(curarg)-2);
                    errflag = O_TRUNC;
                }
                redir_err = 1;
            }
            else{
                internalargv = realloc(internalargv, (internalargc+2)*sizeof(*internalargv));
                internalargv[internalargc] = curarg;
                internalargc++;
            }
        }
        internalargv[internalargc] = (char *)NULL; // argv must be null-terminated

        // Built-in commands
        if(!strcmp(internalargv[0],"cd")){
            switch(internalargc){
                case 2:
                    if(chdir(internalargv[1])<0){
                        fprintf(stderr, "Error: Could not cd to directory %s: %s\n",internalargv[1],strerror(errno));
                        errno = 0;
                    }
                    continue;
                case 1:
                    if(chdir(getenv("HOME"))<0){
                        fprintf(stderr, "Error: Could not cd to directory %s: %s\n",getenv("HOME"),strerror(errno));
                        errno = 0;
                    }
                    continue;
                default:
                    fprintf(stderr, "Error: Too many arguments passed to command cd\n");
                    continue;
            }
        }
        else if(!strcmp(internalargv[0],"pwd")){
            if(internalargc > 1){
                fprintf(stderr, "Error: Too many arguments passed to command pwd\n");
                errno = 0;
            }
            else{
                char cwdname[PATH_MAX];
                if((getcwd(cwdname, PATH_MAX))==NULL){
                    fprintf(stderr,"Error: Call to getcwd failed: %s\n", strerror(errno));
                    errno = 0;
                    continue;
                }
                printf("%s\n",cwdname);    
            }
            continue;
        }
        else if(!strcmp(internalargv[0],"exit")){
            switch(internalargc){
                case 2:
                    errno = 0;
                    long rval = strtol(internalargv[1],NULL,10);
                    if(errno){
                        fprintf(stderr, "Error: Could not convert exit argument to long using strtol.\n");
                        continue;
                    }
                    return (int)rval;
                case 1:
                    return lastexitstatus;
                default:
                    fprintf(stderr, "Error: Too many arguments passed to command exit\n");
                    continue;
            }
        }
        else{
            int pid;
            // Used to track real time
            struct timeval starttime;
            gettimeofday(&starttime, NULL);
            switch(pid = fork()){
                case -1:
                    fprintf(stderr, "Error occured while attempting to fork: %s\n", strerror(errno));
                    continue;
                case 0: // Child Process
                    if(argc == 2){fclose(scriptfile);} // Scriptfile is not needed in child if opened previously
                    int infd = 0, outfd = 1, errfd = 2;
                    if(redir_in && (infd = redirectStd(0, infile, O_RDONLY)) < 0){exit(1);}
                    if(redir_out && (outfd = redirectStd(1, outfile, O_WRONLY | O_CREAT | outflag)) < 0){exit(1);}
                    if(redir_err && (errfd = redirectStd(2, errfile, O_WRONLY | O_CREAT | errflag)) < 0){exit(1);}
                    if(execvp(internalargv[0],internalargv) < 0){
                        fprintf(stderr, "Error attempting to exec process %s: %s\n", internalargv[0], strerror(errno));
                        free(internalargv);
                        exit(127);
                    }
                    break;
                default: // Parent Process
                    free(infile);
                    free(outfile);
                    free(errfile);
                    free(internalargv); // We only need internalargv and redirected files for child, free immediately to minimize leaks
                    struct rusage rusg;
                    unsigned status;
                    int cpid;
                    if((cpid = wait3(&status, 0, &rusg)) < 0){
                        fprintf(stderr, "Error while attempting to wait for child: %s\n", strerror(errno));
                        errno = 0;
                        continue;
                    }
                    if(status != 0){
                        if(WIFSIGNALED(status)){
                            fprintf(stderr, "Child process %d exited with signal %d: %s\n", cpid, WTERMSIG(status), strsignal(WTERMSIG(status)));
                            lastexitstatus = WTERMSIG(status)+128; // Need to add 128 to denote exit caused by signal
                        }
                        else{
                            fprintf(stderr, "Child process %d exited with exit code %d\n", cpid, WEXITSTATUS(status));
                            lastexitstatus = WEXITSTATUS(status);
                        }
                    }
                    else{
                        fprintf(stderr, "Child process %d exited normally\n", cpid);
                        lastexitstatus = 0;
                    }
                    struct timeval endtime, *difftime;
                    gettimeofday(&endtime, NULL);
                    difftime = timediff(starttime, endtime);
                    fprintf(stderr, "Real: %ld.%.6lds User: %ld.%.6lds Sys: %ld.%.6lds\n", difftime->tv_sec, difftime->tv_usec, rusg.ru_utime.tv_sec, rusg.ru_utime.tv_usec, rusg.ru_stime.tv_sec, rusg.ru_stime.tv_usec);

                    break;
            }
        }
    }
    if(errno){
        fprintf(stderr, "Error while trying to read from shell script %s: %s\n", "standard input", strerror(errno));
    }
    else{
        fprintf(stderr, "End of file read, exiting shell with exit code %d\n", lastexitstatus);
    }
    if(argc == 2){fclose(scriptfile);} // Scriptfile is not needed in child if opened previously
    return lastexitstatus;
}