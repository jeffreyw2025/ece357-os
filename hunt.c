# include <stdio.h>
# include <string.h>
# include <errno.h>
# include <fcntl.h>
# include <unistd.h>
# include <dirent.h>
# include <sys/stat.h>

// hunt.c
// By: Jeffrey Wong
/* This program accepts a regular file and a directory, and attempts to find
files that are "identical" (every byte matches) to the given file in the given
directory and all of its subdirectories. */

// Function Declarations
int processDirectory(char *dname, char *targetname, struct stat *targetstats);
int compareFile(char *targetname, char *candidatename);

int main(int argc, char* argv[]){
    if(argc < 3){
        fprintf(stderr, "ERROR: Too few arguments passed to hunt.");
        return -1;
    }
    // Checking and statting initial file
    char *targetfile = argv[1];
    int targetfd;
    struct stat targetstats;

    if((targetfd = open(targetfile, O_RDONLY)) < 0){
        fprintf(stderr, "ERROR: Could not open target file %s for reading: %s\n", targetfile, strerror(errno));
        exit(-1);
    }
    if(fstat(targetfd, &targetstats)){
        fprintf(stderr, "ERROR: Could not stat target file %s: %s\n", targetfile, strerror(errno));
        exit(-1);        
    }
    close(targetfd);

    // Prechecking and processing starting directory
    char *startdir = argv[2];
    DIR *dirp;
    if(!(dirp=opendir(startdir))){
        fprintf(stderr, "ERROR: Could not open starting directory %s: %s\n", startdir, strerror(errno));
        exit(-1);
    }
    processDirectory(startdir, targetfile, &targetstats);
    closedir(dirp);
    return 0;
}

// Recursive function used to traverse a directory to find hits for a given target file descriptor
// Returns 0 on successful execution, or 1 if there is a non-fatal error. Exits program with status -1 if there is a fatal error
int processDirectory(char *dname, char *targetname, struct stat *targetstats){
    DIR *dirp;
    struct dirent *de;
    if(!(dirp=opendir(dname))){
        fprintf(stderr, "Warning: Could not open directory %s: %s\n", dname, strerror(errno));
        errno = 0;
        return 1;
    }
    while(de = readdir(dirp)){
        // Create entry name for processing
        char *entname = strdup(dname);
        strcat(strcat(entname,"/"),de->d_name);
        struct stat candidatestats;
        lstat(entname, &candidatestats); // We don't want to traverse through symlinks just yet
        int type = (candidatestats.st_mode & S_IFMT); // Pull out type so we can process entry correctly
        switch(type){
            case S_IFDIR:
                int i;
                if(!(strcmp(de->d_name,".") && strcmp(de->d_name,"..")) ){break;} // We don't want to traverse through the . or .. entries to prevent infinite loop
                if((i = (processDirectory(entname,targetname,targetstats))) == -1){
                    exit(-1);
                } 
                break;
            case S_IFREG:
                if(targetstats->st_size == candidatestats.st_size){
                    int j;
                    if(targetstats->st_dev == candidatestats.st_dev && targetstats->st_ino == candidatestats.st_ino && targetstats->st_nlink > 1){
                        printf("%s\tHARD LINK TO TARGET\n", entname);
                        break;
                    }
                    else if((j = compareFile(targetname, entname)) == 1){
                        printf("%s\tDUPLICATE OF TARGET (nlink=%d)\n", entname, candidatestats.st_nlink);
                    }
                }
                break;
            case S_IFLNK:
                struct stat linkedstats;
                stat(entname,&linkedstats); // Retreives the data of the linked entry at this point
                if(targetstats->st_size == linkedstats.st_size){
                    int j;
                    if(targetstats->st_dev == linkedstats.st_dev && targetstats->st_ino == linkedstats.st_ino){
                        printf("%s\tSYMLINK RESOLVES TO TARGET\n", entname);
                        break;
                    }
                    else if((j = compareFile(targetname, entname)) == 1){
                        char linkedname[4096];
                        if(readlink(entname, linkedname, 4096) < 0){
                            fprintf(stderr, "Warning: Symlink could not be read: %s\n", strerror(errno));
                            errno = 0;
                            break;
                        }
                        printf("%s\tSYMLINK (%s) RESOLVES TO DUPLICATE\n", entname, linkedname);
                    }
                }
                break;
            default:
                break;
        }
        free(entname);
    }
    if(errno){
        errno = 0;
        return 1;
    }
    closedir(dirp);
    return 0;
}

// Function that compares the target file to a candidate file. It is assumed that they are of equal size beforehand,
// and in this program this check is made before calling this function. Returns 1 if the files match, 0 if they do not match,
// and -1 if there is a fatal error (probably involving the target file)
int compareFile(char *targetname, char *candidatename){

    int targetfd, candidatefd;
    // Buffers for reading target and candidate, respectively, as well as tracker ints
    char buftar[4096];
    char bufcan[4096];
    int i, j;

    // Tedious error handling
    if((targetfd = open(targetname, O_RDONLY)) < 0){
        fprintf(stderr, "ERROR: Could not open target file %s for reading: %s\n", targetname, strerror(errno));
        exit(-1);
    }
    if((candidatefd = open(candidatename, O_RDONLY)) < 0){
        fprintf(stderr, "Warning: Could not open candidate file %s for reading: %s\n", candidatename, strerror(errno));
        errno = 0;
        return 0;
    }

    while((i = read(targetfd, buftar, sizeof(buftar))) > 0){
        if(i < 0){
            fprintf(stderr, "ERROR: Error occured while reading target file %s: %s\n", targetname, strerror(errno));
            exit(-1);
        }
        if((j = read(candidatefd, bufcan, sizeof(bufcan))) != i){
            if(j < 0){
                fprintf(stderr, "Warning: Error occured while reading candidate file %s: %s\n", candidatename, strerror(errno));
                errno = 0;
            }
            return 0;
        }
        if(strncmp(buftar,bufcan,i)){return 0;} // strncmp will be nonzero if there is a discrepancy at any point
    }
    close(targetfd);
    close(candidatefd);
    return 1;
}