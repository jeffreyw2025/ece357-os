# include <stdio.h>
# include <stdlib.h>
# include <ctype.h>
# include <errno.h>
# include <fcntl.h>
# include <unistd.h>

// meow.c
// By: Jeffrey Wong
/* This program mimics the functionality of cat- it allows users to write any number of files
(or from standard input) to a specified output file (standard output if not specified) */

int main(int argc, char* argv[]){
    char buf[4096];
    // Variables for output file
    char *outfile = "<standard output>";
    int outfd = 1;
    // Variables for input file
    char *infile;
    int infd;

    // Checking for -o flag and specifying output file
    // Argument parsing thanks to https://www.gnu.org/software/libc/manual/html_node/Example-of-Getopt.html
    int c;
    if((c = getopt(argc, argv, "o:")) >= 0){ // We only need to scan for one flag this time, multiple times would require a while loop
        switch(c){
            case 'o':
                outfile = optarg;
                if((outfd = open(outfile, O_WRONLY|O_CREAT|O_TRUNC, 0666)) < 0){
                    fprintf(stderr, "Error attempting to open file %s for writing: %s\n", outfile, strerror(errno));
                    return -1;
                }
                break;
            case '?':
                if(optopt == 'o')
                    fprintf(stderr, "No file name specified for output.\n");
                else
                    fprintf(stderr, "Unknown option character entered.\n");
                return -1;
            default:
                fprintf(stderr, "%c\n", c);
                fprintf(stderr, "Unknown error encountered while checking for output file.\n");
                return -1;
        }
    }

    // Checking and reading from input files
    for(int i = optind; i < argc; i++){
        memset(buf,0,sizeof(buf)); // Empties out the buffer upon processing a new input file
        int byteswritten = 0;
        int lineswritten = 1; // There is always a first line even without a newline so we start the count at 1
        if(!(strcmp(argv[i],"-"))){
            infile = "<standard input>";
            infd = 0;
        }
        else{
            infile = argv[i];
            if((infd = open(infile, O_RDONLY, 0666)) < 0){
                fprintf(stderr, "Error attempting to open file %s for reading: %s\n", infile, strerror(errno));
                return -1;
            }
        }
        int j = 0;
        while((j = read(infd, buf, sizeof(buf))) > 0){
            for(int i = 0; i < j; i++){
                if(buf[i] == '\n'){lineswritten++;}
            }
            int k = write(outfd, buf, j);
            if(k < 0){
                fprintf(stderr, "Error attempting to write to file %s: %s\n", outfile, strerror(errno));
                return -1;
            }
            // Handling a "parital write" if bytes written < bytes read
            if(k < j && k > 0){
                int remainder = j - k;
                while(remainder > 0){
                    int kk = write(outfd, buf, remainder);
                    if(kk < 0){
                        fprintf(stderr, "Error attempting to write to file %s: %s\n", outfile, strerror(errno));
                        return -1;
                    }
                    if(kk == 0){break;}
                    remainder -= kk;
                }
            }
            byteswritten += j;
        }
        if(j < 0){
            fprintf(stderr, "Error occured while reading file %s: %s\n", infile, strerror(errno));
        }
        fprintf(stderr, "File %s finished reading. %d bytes transferred, %d lines read.\n", infile, byteswritten, lineswritten);
    }
    return 0;
}