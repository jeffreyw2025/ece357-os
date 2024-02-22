# include <stdio.h>
# include <stdlib.h>
# include <ctype.h>
# include <errno.h>
# include <fcntl.h>
# include <unistd.h>

// Our assembly program has buffer in bss region, mimic this
char buf[4096];

int main(){
    int bytesread = read(0, buf, sizeof(buf));
    write(1, buf, bytesread);
    return 0;
}