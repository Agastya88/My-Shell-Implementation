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
        pid_t newProcessPid;
        char uinput[INPUT_MAX];
        //taking a string as input from the user
        char *inputString = fgets(uinput, INPUT_MAX, stdin);
        //need to look at how fgets tells you its reached the end of the
        //file and then leave the loop then
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
        //skipping to next input if their is no input
        if (noOfArguments==0){
            continue;
        }
        //counting the numberOfPipes
        int numberOfPipes = 0;
        for (int i=0; i<noOfArguments; i++){
            if (strcmp (inputStringArgs [i], "|") == 0){
                numberOfPipes++;
            }
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
            //to do piping for multiple pipes, I can keep an array
            //that records the locations of all the pipes in the input
            //string then I can divide up the calls to pipe using
            //those divisions; so supposing I have 5 pipes that means
            //that I would then have 6 commands. I could pipe them in
            //order using a loop.
            //handlles the case wherein the number of pipes > 0
        else{
            printf ("Number of pipes: %d\n", numberOfPipes);
            int locationOfPipe;
            for (int k=0; k<noOfArguments; k++){
                if (strcmp (inputStringArgs [k], "|") == 0){
                    locationOfPipe=k;
                }
            }
            //dividing the input command into its two constituents
            char *programOneArgs [locationOfPipe];
            for (int j=0; j<locationOfPipe; j++){
                programOneArgs [j] = inputStringArgs [j];
            }
            programOneArgs[locationOfPipe] = 0;
            char *programTwoArgs [noOfArguments-locationOfPipe];
            int l=0;
            for (int j=locationOfPipe+1; j<noOfArguments; j++, l++){
                programTwoArgs [l] = inputStringArgs [j];
            }
            programTwoArgs[l] = 0;
            piping (programOneArgs, programTwoArgs);
        }
        wait (NULL);
        wait (NULL);
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
    pipe (fildes);
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
    //TO DO: Ask Pete about the order of the above forks & why one way works
    //and one way does not.
}
