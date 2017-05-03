#ifndef SFISH_H
#define SFISH_H
#include <readline/readline.h>
#include <readline/history.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#define MAX_SIZE 1024

void testMethod(char** arrayStrings);

int getBuiltInCommand(char* commandName);

char** tokenize(char* cmd, char* delims);

void printWorkingDirectory();

void printHelp(char* fileName);

int argsLength(char** args, int pipe);

int changeDirectory(char* path);

int containsChar(char** argv,char character);

int execute(char** argList);

char** constructArgv(char** oldArgV, char** fileName);

char** extractPipe(char** argV, int pipeNo);

int countPipes(char** argV);

int runPipedPrograms(int*fds, char*** commands);

typedef void handler_t(int);

handler_t* Signal(int signum, handler_t* handler);

void alarmHandler(int signum);

int setAlarm(char** argV);

void userHandler(int signum);

void childHandler(int signum, siginfo_t * info, void* ignore);

int checkIfValid(char** argV);

void freeArray(char*** array);

#endif
