#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <linux/limits.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#define INPUT_MAX 4096

void printprompt();
void inputRedirection (char* inputFileName);
void outputRedirection (int outputFile);

int main(int argc, char *argv[]) {
    pid_t shellpid;
    shellpid = getpid();
    printf ("Shell ID: %d \n", shellpid);
    while(1){
        printprompt();
        pid_t childpid;
        char uinput[INPUT_MAX];
        char *inputString = fgets(uinput, INPUT_MAX, stdin);
        //parsing the string they input into an array of arguments
        int i = 0;
        char *p = strtok(inputString, "\n");
        p = strtok(p, " ");
        char *inputStringArgs[10];
        int noOfArguments = 0;
        while (p != NULL){
            inputStringArgs[i++] = p;
            p = strtok(NULL, " ");
            noOfArguments++;
        }
        childpid = fork();
        if(childpid==-1){
            perror("couldn't fork child");
            exit (0);
        }
        else if(childpid==0){
            //checking for exit
            if (noOfArguments!=0){
                if (strcmp (inputStringArgs [0], "exit") == 0){
                    exit (2);
                }
            }
            if (noOfArguments>=3){
                //checking if there is a need for input redirection
                if (strcmp (inputStringArgs [1], "<") == 0){
                    inputRedirection (inputStringArgs[2]);
                }
                //checking if there is a need for output redirection (truncated)
                else if (strcmp (inputStringArgs [1], ">") == 0){
                    int outputFileT = open (inputStringArgs[2], O_WRONLY | O_TRUNC);
                    outputRedirection (outputFileT);
                }
                //checking if there is a need for output redirection (appended)
                else if (strcmp (inputStringArgs [1], ">>") == 0){
                    int outputFileA = open (inputStringArgs[2], O_WRONLY | O_APPEND);
                    outputRedirection (outputFileA);
                }
            }
            inputStringArgs[i] = 0;
            execvp(inputStringArgs[0], inputStringArgs);
        }
        else{
            wait(&childpid);
            if (WIFEXITED(childpid)){
                continue;
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

void inputRedirection (char* inputFileName){
    int inputFile = open (inputFileName, O_RDONLY);
    //checking for errors during opening
    if (inputFile==-1){
        perror("Error:");
    }
    //performing input redirection
    dup2 (inputFile, 0);
    //checking for errors during closing
    if (close (inputFile)==-1){
        perror("Error:");
    }
}

void outputRedirection (int outputFile){
    //checking for errors during opening
    if (outputFile==-1){
        perror("Error:");
    }
    //performing output redirection
    dup2 (outputFile, 1);
    //checking for errors during closing
    int closeRV = close (outputFile);
    if (closeRV==-1){
        perror("Error:");
    }
}
