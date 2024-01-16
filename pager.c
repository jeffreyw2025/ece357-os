# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <errno.h>
# include <fcntl.h>
# include <unistd.h>

// pager.c
// By: Jeffrey Wong
/* Prints up to 23 lines at a time from standard input.
After each batch of 23 lines the user is prompted to press
enter or q or Q. The program terminates upon
reading EOF or when the user presses q or Q on prompt. */

int main(void){
    FILE *term = fopen("/dev/tty", "r");
    int linesread = 0;
    char nextline[4096];
    while(fgets(nextline, 4096, stdin)){
        linesread++;
        printf("%s",nextline);
        if(linesread >= 23){
            unsigned char cmd;
            printf("---Press RETURN for more---");
            while((cmd = (unsigned char)fgetc(term))!='\n' && cmd != 'q' && cmd != 'Q'){}
            if(cmd == 'q' || cmd == 'Q'){
                printf("*** Pager terminated by Q command ***\n");
                return 0;
            }
            linesread = 0;
        }
    }
    if(errno){
        fprintf(stderr, "Error in pager when retreiving line: %s", strerror(errno));
        return -1;
    }
    printf("\n");
    return 0;
}