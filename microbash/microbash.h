#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAXNUMCOMMANDS 10
#define MAXNUMARGS 10

typedef struct
{
    char* command[MAXNUMARGS];
    int numOfArgs;
} Command;

//Function prototypes
void initCmd(Command cmd[], int* numOfCommands);
void deleteCmd(Command cmd[], int* numOfCommands);

void fail_errno(const char * const msg);
void fail(const char *const msg);

void printPrompt();
int getCommand(Command cmd[], int* numOfCommands);
void splitLine(char* line, Command cmd[], int* numOfCommands);
void splitCommand(char *command, Command cmd[], const int cmdNum);
void saveArg(Command cmd[], int i, int j, char *arg);
int checkCmdLine(Command cmd[], const int* numOfCommands);

void execCmdLine(Command cmd[], int *numOfCommands);
void execExternCmd(Command cmd[], int *numOfCommands, const int cmdToExec);
void waitChildren(Command cmd[], int *numOfCommands, pid_t pid[]);
void extractVar(Command cmd[], int *numOfCommands);
int cd(char *directory);

void createPipe(Command cmd[], int *numOfCommands, int pipedes[]);
void adjust_fd(Command cmd[], int *numOfCommands, int pipedes[], int i, int j, int z);
void redirect(Command cmd[], int *numOfCommands, const int i);
int openFileOut(Command cmd[], int *numOfCommands, int i, int j);
int openFileIn(Command cmd[], int *numOfCommands, int i, int j);
void updateArgs(Command cmd[], int i, int j);
void makeDup2(Command cmd[], int *numOfCommands, int oldFd, int newFd);