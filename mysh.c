#include <stdio.h>
#include <unistd.h>
#include <linux/limits.h>

#define INPUT_MAX 4096

void printprompt();

int main(int argc, char *argv[]) {   
    pid_t shellpid;
    shellpid = getpid();
    while(1) {
        printprompt();
        pid_t childpid;
        char uinput[INPUT_MAX];
        fgets(uinput, INPUT_MAX, stdin);
        childpid = fork();
        if (childpid == 0){
            //remove newline at end of uinput?
            //exec uinput
        } else{
            wait();
        }
    }
}

void printprompt(){
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL){
        printf("%s$ ", cwd);
    }
}
