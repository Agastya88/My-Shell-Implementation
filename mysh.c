#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <linux/limits.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#define INPUT_MAX 4096

void printprompt();

int main(int argc, char *argv[]) {
    pid_t shellpid;
    shellpid = getpid();
    printf ("Shell ID: %d \n", shellpid); //debugging purposes
    while(1){
        printprompt();
        pid_t childpid;
        char uinput[INPUT_MAX];
        char *inputString = fgets(uinput, INPUT_MAX, stdin);
        childpid = fork();
        if(childpid==-1){
            perror("couldn't fork child");
            exit (0);
        }
        else if(childpid==0){
            //parsing the string they input into an array of arguments
            int i = 0;
            char *p = strtok(inputString, "\n");
            p = strtok(p, " ");
            char *inputStringArgs[10];
            while (p != NULL){
                inputStringArgs[i++] = p;
                p = strtok(NULL, " ");
            }
            inputStringArgs[i] = 0;
            if (strcmp(inputStringArgs[0], "exit") == 0) {
                exit(1);
            }
            execvp(inputStringArgs[0], inputStringArgs);
        }
        else{
            wait(&childpid);
            if (WIFEXITED(childpid)){
                exit(1);
            }
        }
    }
}

void printprompt(){
    //uncomment below to print cwd before $
    /*
       char cwd[PATH_MAX];
       if(getcwd(cwd, sizeof(cwd)) != NULL){
       printf("%s$ ", cwd);
       }
     */
    printf("$");
}
