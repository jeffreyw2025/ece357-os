# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <errno.h>
# include <fcntl.h>
# include <unistd.h>
# include <setjmp.h>
# include <sys/signal.h>
# include <sys/types.h>

// wordsearch.c
// By: Jeffrey Wong
/* The program accepts one file as an argument and reads in one line at a time
from that file, adding each line as an entry to a dictionary. It then reads lines
from standard input to see if the line matches any entry in the dictionary, then
prints out the number of matches found. */

// Struct definitions
struct dict{
    char **validwords;
    int entries;
    int capacity;
};

// Global variables

jmp_buf int_jb;

// Function Definitions

void pipeHandler(int sn){
    longjmp(int_jb,1);
}


char* toUpper(char *str){
    char temp[strlen(str)+1];
    for(int i = 0; i < strlen(str); i++){
        if(str[i] >= 'a' && str[i] <= 'z'){temp[i] = str[i]-32;}
        else{temp[i] = str[i];}
    }
    temp[strlen(str)]='\0'; // Null terminate string
    char *result = strdup(temp);
    return result;
}

int main(int argc, char **argv){
    // Initialize Dictionary
    struct dict dictionary;
    dictionary.entries = 0;
    dictionary.capacity = 10;
    dictionary.validwords = malloc(dictionary.capacity * sizeof (char*));

    // Open file for reading
    if(argc < 2){
        fprintf(stderr, "Error: Not enough arguments supplied to wordsearch.\n");
        return -1;
    }
    FILE *dictwords;
    if((dictwords = fopen(argv[1], "r"))==NULL){
        fprintf(stderr, "Error: Could not open file %s for reading\n", argv[1]);
        return -1;
    }

    char entry[4096];
    while(fgets(entry, 4096, dictwords)){
        if(dictionary.entries >= dictionary.capacity){
            dictionary.capacity *= 2; 
            if((dictionary.validwords = realloc(dictionary.validwords, dictionary.capacity * sizeof (char*))) == NULL){
                fprintf(stderr, "Error: Could not allocate sufficient memory to dictionary\n");
                return -1;
            }
        }
        char *entryUpper = toUpper(entry);
        dictionary.validwords[dictionary.entries] = entryUpper;
        dictionary.entries++;
    }
    if(errno){
        fprintf(stderr, "Error: Could not read from file %s\n", argv[1]);
    }

    char candidate[4096];
    int numMatches = 0;
    signal(SIGPIPE, pipeHandler);
    while(fgets(candidate, 4096, stdin)){
        for(int i = 0; i < dictionary.entries; i++){
            if(!strcmp(candidate,dictionary.validwords[i])){
                printf("%s",candidate);
                numMatches++;
                break; // We'll just check for the first match. Ignore redundancy
            }
        }
        /* Check after each word if there is a SIGPIPE to handle. Since we exit immediately after SIGPIPE and the below cleanup,
        the fact that the mask is set after the longjmp is actually a good thing. */
        if(setjmp(int_jb)!=0){
            break;
        }
    }
    free(dictionary.validwords); // Memory cleanup
    fprintf(stderr, "Matched %d words\n", numMatches);
    return 0;
}