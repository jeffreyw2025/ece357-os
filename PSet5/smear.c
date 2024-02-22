# include <stdio.h>
# include <stdlib.h>
# include <fcntl.h>
# include <string.h>
# include <errno.h>
# include <unistd.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <sys/mman.h>


// smear.c
// By: Jeffrey Wong
/* Accepts a target string, a destination string of the same length as the target, and any number of files. 
In each file, smear replaces every instance of the target string with the destination string. */

int main(int argc, char** argv){
    // Preliminary error handling
    if(argc < 4){
        fprintf(stderr, "Error: Insufficient arguments passed to smear\n");
        return -1;
    }
    char *target = argv[1];
    char *dest = argv[2];
    if(strlen(target) != strlen(dest)){
        fprintf(stderr, "Error: Target string %s and destination string %s not the same length\n", target, dest);
        return -1;
    }
    int curfd;
    for(int i = 3; i < argc; i++){
        if((curfd = open(argv[i], O_RDWR)) < 0){
            fprintf(stderr, "Error attempting to open file %s for read/write: %s\n", argv[i], strerror(errno));
            return -1;
        }

        // Retrieve size of file and mmap file
        struct stat candidatestats;
        if(fstat(curfd, &candidatestats) < 0){
            fprintf(stderr, "Error attempting to stat file %s: %s\n", argv[i], strerror(errno));
            return -1;
        }
        int fsize = candidatestats.st_size;

        char *addr;
        if((addr = mmap(NULL, fsize, PROT_READ | PROT_WRITE, MAP_SHARED, curfd, 0)) < 0){
            fprintf(stderr, "Error attempting to map file %s to memory region: %s\n", argv[i], strerror(errno));
            return -1;
        }

        // If the remaining length is less than the target string length, it is not possible to replace so we skip
        for(int j = 0; j <= fsize - strlen(target); j++){
            // We unfortunately have to do the string comparison and replacement in a tedious, manual manner, most string methods
            // don't play nice with our mmaped region
            int matches = 1;
            for(int k = 0; k < strlen(target); k++){
                if(addr[j+k] != target[k]){
                    matches = 0;
                    break;
                }
            }
            if(matches){
                for(int k = 0; k < strlen(dest); k++){
                    addr[j+k] = dest[k];
                }
                j += (strlen(dest)-1); // If we have a match, we can skip over all the characters we replace 
                // (combined with the j++ we skip strlen(target) chars)
            }
        }

        // Cleanup routine
        munmap(addr, fsize);
        close(curfd);
    }
    return 0;
}