#include <stdio.h>

int main(int argc, char *argv[]) {
    //input redirection testing
    char *buf;
    char *ret = fgets (buf, 10, stdin);
    printf ("Buf: %s\n", buf);
    //output redirection testing
    printf ("My name is Agastya\n");
}
