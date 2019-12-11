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
        char *inputStringArgs[strlen(inputStringCopy)];
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
        int lengthOfInput = strlen (inputStringCopy);
        for (int i=0; i<lengthOfInput;i++){
            if (inputString[i] == '|')
                numberOfPipes++;
        }
        //handles a single command i.e. commands that do not include piping
        if (numberOfPipes == 0){
            newProcessPid = fork();
            if(newProcessPid==-1){
                perror("couldn't fork child");
            }
            else if(newProcessPid==0){
                int redirectionLocation = noOfArguments;
                //stores the location of redirection
                for (int i=0; i<noOfArguments; i++){
                    //checking if there is a need for input redirection
                    if (strcmp (inputStringArgs [i], "<") == 0){
                        if (i<redirectionLocation){
                            redirectionLocation = i;
                            inputRedirection (inputStringArgs[i+1]);
                        }
                    }
                    //checking if there is a need for output redirection (truncated)
                    else if (strcmp (inputStringArgs [i], ">") == 0){
                        int outputFileT = open (inputStringArgs[i+1], O_CREAT| O_WRONLY | O_TRUNC);
                        if (i<redirectionLocation){
                            redirectionLocation = i;
                            outputRedirection (outputFileT);
                        }
                    }
                    //checking if there is a need for output redirection (appended)
                    else if (strcmp (inputStringArgs [i], ">>") == 0){
                        int outputFileA = open (inputStringArgs[i+1], O_CREAT | O_WRONLY | O_APPEND);
                        if (i<redirectionLocation){
                            redirectionLocation = i;
                            outputRedirection (outputFileA);
                        }
                    }
                }
                char *commandArgs [redirectionLocation];
                for (int i=0; i<redirectionLocation; i++){
                    commandArgs [i] = inputStringArgs [i];
                }
                commandArgs[redirectionLocation] = 0;
                if (execvp(commandArgs[0], commandArgs) == -1){
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
            char *pipedCommands[numberOfPipes+1];
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
    //checking for errors during opening
    int inputFile = open (inputFileName, O_RDONLY);
    if (inputFile==-1){
        perror("Error:");
        exit (2);
    }
    //performing input redirection
    if (dup2 (inputFile, 0) == -1){
        perror ("Error: ");
        exit (2);
    }
    //checking for errors during closing
    if (close (inputFile)==-1){
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
    if (dup2 (outputFile, 1) == -1){
        perror ("Error: ");
        exit (2);
    }
    //checking for errors during closing
    if (close (outputFile)==-1){
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
        if (pipe (pipefds + i*2) == -1){
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
                if (dup2(pipefds[(currentCommand-1)*2], 0) == -1){
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
                if (dup2(pipefds[currentCommand*2+1], 1) == -1){
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
                if (close (pipefds[i]) == -1){
                    perror ("Error: ");
                    exit (3);
                }
            }
            if (execvp (pipedCommandArgs[0], pipedCommandArgs) == -1){
                perror ("Error: ");
                exit (3);
            }
            //resetting arguments
            memset(pipedCommandArgs, '\0', sizeof(pipedCommandArgs));
        }
        else if (pid==-1){
            perror ("Error: ");
        }
    }
    //closing all of the pipes in the parent
    for(int i=0; i<noOfPipeEnds; i++){
        if (close(pipefds[i]) == -1){
            perror ("Error: ");
            exit (3);
        }
    }
    //waiting for all the child processes to complete
    for (int i=0; i<processCount; i++){
        if (wait(NULL) == -1){
            perror ("Error: ");
            exit (3);
        }
    }
}
