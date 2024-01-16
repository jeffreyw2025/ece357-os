# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <time.h>
# include <errno.h>

// wordgen.c
// By: Jeffrey Wong
/* Generates an assigned number of words consisting of 3 to 5 uppercase characters. 
This program can accept up to 1 argument. If a positive integer is supplied, it generates
that number of words, otherwise it generates words until the program is forcibly terminated */

int generate_word(int len){
    char word[len+1];
    for(int i = 0; i < len; i++){
        word[i] = 'A' + (rand() % 26);
    }
    word[len] = '\0';
    printf("%s\n",word);
}

int main(int argc, char *argv[]){
    long numwords = 0, candidate, wordsgenerated = 0;
    int minlength = 3, maxlength = 10;
    if(argc > 1){
        candidate = strtol(argv[1], NULL, 10);
        if(errno){
            fprintf(stderr, "Error: Argument to wordgen invalid: %s", strerror(errno));
            return -1;
        }
        numwords = candidate > 0 ? candidate : 0;
    }
    srand(time(NULL));
    if(numwords > 0){
        for(int i = 0; i < numwords; i++){
            int wordlength = minlength + (rand() % (maxlength-minlength+1)); // Word length is fixed to max if we call this inside for some reason
            generate_word(wordlength); // This will generate a random length from 3 to 5
            wordsgenerated++;
        }
        fprintf(stderr, "Finished generating %ld candidate words\n", wordsgenerated);
    }
    else{
        for(;;){ // This will generate words infinitely
            int wordlength = minlength + (rand() % (maxlength-minlength+1));
            generate_word(wordlength);
            wordsgenerated++;
        }
    }
    return 0;
}
