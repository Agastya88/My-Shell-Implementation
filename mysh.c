#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <linux/limits.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>

#define INPUT_MAX 4096

void printprompt();
void inputRedirection (char *inputFileName);
void outputRedirection (int outputFile);
void singlePiping (char *pipedCommands[]);
void multiplePiping (char *pipedCommands[], int numberOfPipes);
void piping (char *pipedCommands[], int numberOfPipes);

int main(int argc, char *argv[]) {
    pid_t shellpid;
    shellpid = getpid();
    printf ("Shell ID: %d \n", shellpid);
    while(1){
        printprompt();
        pid_t newProcessPid;
        char uinput[INPUT_MAX];
        //taking a string as input from the user
        char *inputString = fgets(uinput, INPUT_MAX, stdin);
        //checking if they used Control+D to terminate
        if (inputString == NULL){
            exit (2);
        }
        //creating a copy for later usage (during piping section);
        char *inputStringCopy = malloc(sizeof(inputString));
        strcpy (inputStringCopy, inputString);
        //parsing the string they input into an array of arguments
        char *p = strtok(inputString, "\n");
        //the first token; tokenized using a space
        p = strtok(p, " ");
        char *inputStringArgs[10];
        int noOfArguments = 0;
        while (p != NULL){
            inputStringArgs[noOfArguments++] = p;
            //tokenizing by spaces
            p = strtok(NULL, " ");
        }
        //checking if they typed the command exit to terminate
        if (noOfArguments!=0){
            if (strcmp (inputStringArgs [0], "exit") == 0){
                exit (2);
            }
        }
        //skipping to next input if their is no input
        if (noOfArguments==0){
            continue;
        }
        //counting the numberOfPipes
        int numberOfPipes = 0;
        for (int i=0; i<strlen(inputStringCopy);i++){
            if (inputString[i] == '|')
                numberOfPipes++;
        }
        int noOfProcesses = numberOfPipes+1;
        //handles a single command i.e. commands that do not include piping
        if (numberOfPipes == 0){
            newProcessPid = fork();
            if(newProcessPid==-1){
                perror("couldn't fork child");
                exit (0);
            }
            else if(newProcessPid==0){
                int rL = noOfArguments;
                //stores the location of redirection
                for (int i=0; i<noOfArguments; i++){
                    //checking if there is a need for input redirection
                    if (strcmp (inputStringArgs [i], "<") == 0){
                        if (i<rL){
                            rL = i;
                            inputRedirection (inputStringArgs[i+1]);
                        }
                    }
                    //checking if there is a need for output redirection (truncated)
                    else if (strcmp (inputStringArgs [i], ">") == 0){
                        int outputFileT = open (inputStringArgs[i+1], O_WRONLY | O_TRUNC);
                        if (i<rL){
                            rL = i;
                            outputRedirection (outputFileT);
                        }
                    }
                    //checking if there is a need for output redirection (appended)
                    else if (strcmp (inputStringArgs [i], ">>") == 0){
                        int outputFileA = open (inputStringArgs[i+1], O_WRONLY | O_APPEND);
                        if (i<rL){
                            rL = i;
                            outputRedirection (outputFileA);
                        }
                    }
                }
                char *commandArgs [rL];
                for (int i=0; i<rL; i++){
                    commandArgs [i] = inputStringArgs [i];
                }
                commandArgs[rL] = 0;
                execvp(commandArgs[0], commandArgs);
            }
            else{
                wait (NULL);
                if (WIFEXITED(newProcessPid)){
                    //resetting arguments
                    memset(inputStringArgs, '\0', sizeof(inputStringArgs));
                    continue;
                }
            }
        }
        //handlles the case wherein there is a notion of piping
        else{
            //parsing the input string by pipes to find the commands
            //that will give rise to processes
            char *q = strtok(inputStringCopy, "\n");
            q = strtok(q, "|");
            char *pipedCommands[5];
            int noOfCommands = 0;
            while (q != NULL){
                pipedCommands[noOfCommands++] = q;
                q = strtok(NULL, "|");
            }
            multiplePiping (pipedCommands, numberOfPipes);
        }
        for (int k=0; k<noOfProcesses; k++){
            wait (NULL);
        }
    }
}

void printprompt(){
    //puts the current working directory before the $
    //char cwd[PATH_MAX];
    /*if(getcwd(cwd, sizeof(cwd)) != NULL){
        printf("%s$ ", cwd);
    }*/
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
    int closeRV = close (inputFile);
    if (closeRV==-1){
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

void singlePiping (char *pipedCommands[]){
    int fildes[2];
    pipe (fildes);
    //getting the arguments for the first program
    char *p = strtok(pipedCommands[0], "\0");
    p = strtok(p," ");
    char *programOne[10];
    int pOneArgCount = 0;
    while (p != NULL){
        programOne[pOneArgCount++] = p;
        p = strtok(NULL, " ");
    }
    //getting the arguments for the second program
    char *q = strtok(pipedCommands[1], "\0");
    q = strtok(q," ");
    char *programTwo[10];
    int pTwoArgCount = 0;
    while (q != NULL){
        programTwo[pTwoArgCount++] = q;
        q = strtok(NULL, " ");
    }
    //handles the write end of the pipe
    if (fork () == 0){
        //close the read end of the pipe
        close (fildes[0]);
        //replace stdout with write end of pipe
        dup2 (fildes[1], 1);
        //execute the program
        execvp (programOne [0], programOne);
    }
    //handling the read end of the pipe
    if (fork () == 0){
        //close the write end of the pipe
        close (fildes[1]);
        //replace stdin with read end of pipe
        dup2 (fildes[0], 0);
        //execute the program
        execvp (programTwo [0], programTwo);
    }
}

void multiplePiping (char *pipedCommands[], int numberOfPipes){
    int processCount = numberOfPipes+1;
    int noOfPipeEnds = numberOfPipes*2;
    int pipefds[noOfPipeEnds];
    //creating all the necessary pipes in the parent
    for (int i=0; i<numberOfPipes; i++){
        pipe (pipefds + i*2);
    }
    //dup-ing and executing for each command
    for (int currentCommand=0; currentCommand<processCount; currentCommand++){
        int pid = fork();
        if (pid== 0){
            //getting input from previous command (if there is one)
            if(currentCommand!=0){
                dup2(pipefds[(currentCommand-1)*2], 0);
            }
            //checking for input redirection in the first command
            else{
                //loop through the command and look for <
                //perform ID if it is present
            }
            //outputting to next command (if it exists)
            if (currentCommand!=processCount-1){
                dup2(pipefds[currentCommand*2+1], 1);
            }
            //checking for output redirection in the last command
            else{
                //loop through the command and look for > or >>
                //perform OD if it is presentom
            }
            //closing all the pipe ends
            for (int i=0; i<noOfPipeEnds; i++){
                close (pipefds[i]);
            }
            //getting the arguments of the current command
            char *r = strtok(pipedCommands[currentCommand], "\0");
            r = strtok(r," ");
            char *pipedCommandArgs[10];
            int noOfCommandArgs = 0;
            while (r != NULL){
                pipedCommandArgs[noOfCommandArgs++] = r;
                r = strtok(NULL, " ");
            }
            execvp (pipedCommandArgs[0], pipedCommandArgs);
        }
        else if (pid==-1){
            perror ("Error: ");
            exit (3);
        }
    }
    //closing all of the pipes in the parent
    for(int i=0; i<noOfPipeEnds; i++){
        close(pipefds[i]);
    }
    //waiting for all the child processes to complete
    for (int i=0; i<processCount; i++){
        wait(NULL);
    }
}
