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
        char *inputStringCopy = strdup(inputString);
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
                int execRV = execvp(commandArgs[0], commandArgs);
                if (execRV == -1){
                    perror ("Error: ");
                    exit (3);
                }
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
            free (inputStringCopy);
        }
    }
}

void printprompt(){
    //puts the current working directory before the $
    char cwd[PATH_MAX];
    if(getcwd(cwd, sizeof(cwd)) != NULL){
        printf("%s$ ", cwd);
    }
}

void inputRedirection (char *inputFileName){
    int inputFile = open (inputFileName, O_RDONLY);
    //checking for errors during opening
    if (inputFile==-1){
        perror("Error:");
        exit (2);
    }
    //performing input redirection
    int dupRV = dup2 (inputFile, 0);
    if (dupRV == -1){
        perror ("Error: ");
        exit (2);
    }
    //checking for errors during closing
    int closeRV = close (inputFile);
    if (closeRV==-1){
        perror("Error: ");
        exit (2);
    }
}

void outputRedirection (int outputFile){
    //checking for errors during opening
    if (outputFile==-1){
        perror("Error:");
        exit (2);
    }
    //performing output redirection
    int dupRV = dup2 (outputFile, 1);
    if (dupRV == -1){
        perror ("Error: ");
        exit (2);
    }
    //checking for errors during closing
    int closeRV = close (outputFile);
    if (closeRV==-1){
        perror("Error:");
        exit (2);
    }
}

void multiplePiping (char *pipedCommands[], int numberOfPipes){
    int processCount = numberOfPipes+1;
    int noOfPipeEnds = numberOfPipes*2;
    int pipefds[noOfPipeEnds];
    //creating all the necessary pipes in the parent
    for (int i=0; i<numberOfPipes; i++){
        int pipeRV = pipe (pipefds + i*2);
        if (pipeRV == -1){
            perror ("Error: ");
            exit (3);
        }
    }
    //dup-ing and executing for each command
    for (int currentCommand=0; currentCommand<processCount; currentCommand++){
        int pid = fork();
        if (pid== 0){
            //getting the arguments of the current command
            char *r = strtok(pipedCommands[currentCommand], "\0");
            r = strtok(r," ");
            char *pipedCommandArgs[10];
            for (int i=0; i<10; i++){
                pipedCommandArgs [i] = '\0';
            }
            int noOfCommandArgs = 0;
            while (r != NULL){
                pipedCommandArgs[noOfCommandArgs++] = r;
                r = strtok(NULL, " ");
            }
            //getting input from previous command (if there is one)
            if(currentCommand!=0){
                int dupRV = dup2(pipefds[(currentCommand-1)*2], 0);
                if (dupRV == -1){
                    perror ("Error: ");
                    exit (3);
                }
            }
            //checking for input redirection in the first command
            else{
                //loop through the command and look for <
                for (int i=0; i<noOfCommandArgs; i++){
                    if (strcmp(pipedCommandArgs[i], "<") == 0){
                        //perform ID if it is present
                        inputRedirection (pipedCommandArgs[i+1]);
                    }
                }
            }
            //outputting to next command (if it exists)
            if (currentCommand!=processCount-1){
                int dupRV = dup2(pipefds[currentCommand*2+1], 1);
                if (dupRV == -1){
                    perror ("Error: ");
                    exit (3);
                }
            }
            //checking for output redirection in the last command
            else{
                //loop through the command and look for > or >>
                for (int i=0; i<noOfCommandArgs; i++){
                    if (strcmp(pipedCommandArgs[i], ">") == 0){
                        int outputFileT = open (pipedCommandArgs[i+1], O_WRONLY | O_TRUNC);
                        //perform OD if it is present
                        outputRedirection(outputFileT);
                    }
                    else if (strcmp(pipedCommandArgs[i], ">>") == 0){
                        int outputFileA = open (pipedCommandArgs[i+1], O_WRONLY | O_APPEND);
                        //perform OD if it is present
                        outputRedirection(outputFileA);
                    }
                }
            }
            //closing all the pipe ends
            for (int i=0; i<noOfPipeEnds; i++){
                int closeRV = close (pipefds[i]);
                if (closeRV == -1){
                    perror ("Error: ");
                    exit (3);
                }
            }
            int execRV = execvp (pipedCommandArgs[0], pipedCommandArgs);
            if (execRV == -1){
                perror ("Error: ");
                exit (3);
            }
            //resetting arguments
            memset(pipedCommandArgs, '\0', sizeof(pipedCommandArgs));
        }
        else if (pid==-1){
            perror ("Error: ");
            exit (3);
        }
    }
    //closing all of the pipes in the parent
    for(int i=0; i<noOfPipeEnds; i++){
        int closeRV = close(pipefds[i]);
        if (closeRV == -1){
            perror ("Error: ");
            exit (3);
        }
    }
    //waiting for all the child processes to complete
    for (int i=0; i<processCount; i++){
        int waitRV = wait(NULL);
        if (waitRV == -1){
            perror ("Error: ");
            exit (3);
        }
    }
}
