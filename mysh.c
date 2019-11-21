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
void inputRedirection (char *inputFileName);
void outputRedirection (int outputFile);
void piping (char *programOne[], char *programTwo[]);

int main(int argc, char *argv[]) {
    pid_t shellpid;
    shellpid = getpid();
    printf ("Shell ID: %d \n", shellpid);
    while(1){
        printprompt();
        pid_t childpid; //need to rename this var
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
        //checking for exit
        if (noOfArguments!=0){
            if (strcmp (inputStringArgs [0], "exit") == 0){
                exit (2);
            }
        }
        childpid = fork();
        if(childpid==-1){
            perror("couldn't fork child");
            exit (0);
        }
        else if(childpid==0){
            int rL = noOfArguments;
            int numberOfPipes = 0;
            //stores where the first redirection lies
            for (int i=0; i<noOfArguments; i++){
                //checking if there is a need for input redirection
                if (strcmp (inputStringArgs [i], "<") == 0){
                    inputRedirection (inputStringArgs[i+1]);
                    if (i<rL){
                        rL = i;
                    }
                }
                //checking if there is a need for output redirection (truncated)
                else if (strcmp (inputStringArgs [i], ">") == 0){
                    int outputFileT = open (inputStringArgs[i+1], O_WRONLY | O_TRUNC);
                    outputRedirection (outputFileT);
                    if (i<rL){
                        rL = i;
                    }
                }
                //checking if there is a need for output redirection (appended)
                else if (strcmp (inputStringArgs [i], ">>") == 0){
                    int outputFileA = open (inputStringArgs[i+1], O_WRONLY | O_APPEND);
                    outputRedirection (outputFileA);
                    if (i<rL){
                        rL = i;
                    }
                }
                //checking if there is a call for piping
                //NOTE: this is only going to work for 1 pipe right now.
                else if (strcmp (inputStringArgs [i], "|") == 0){
                    char *programOneArgs [i];
                    for (int j=0; j<i; j++){
                        programOneArgs [j] = inputStringArgs [i];
                    }
                    programOneArgs[i] = 0;
                    int k=10-i;
                    char *programTwoArgs [k];
                    for (int j=i; j<k; j++){
                        programTwoArgs [j] = inputStringArgs [i];
                    }
                    programTwoArgs[k] = 0;
                    piping (programOneArgs, programTwoArgs);
                    numberOfPipes++;
                }
            }
            if (numberOfPipes==0){
                char *commandArgs [rL];
                for (int i=0; i<rL; i++){
                    commandArgs [i] = inputStringArgs [i];
                }
                commandArgs[rL] = 0;
                execvp(commandArgs[0], commandArgs);
            }
            printf ("number of pipes are: %d\n", numberOfPipes);
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

void inputRedirection (char *inputFileName){
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

void piping (char *programOne[], char *programTwo[]){
    int fildes[2];
    int pid;
    pipe (fildes);
    pid = fork ();
    //handles the read end of the pipe
    if (pid == 0){
        //replace stdin with read end of pipe
        dup2 (fildes[0], 0);
        close (fildes[1]);
        //execute the program
        execvp (programTwo [0], programTwo);
    }
    //handling the write end of the pipe
    else{
        //replace stdout with write end of pipe
        dup2 (fildes[1], 1);
        close (fildes[0]);
        //execute the program
        execvp (programOne [0], programOne);
    }
}
