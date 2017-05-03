#include "sfish.h"
#include "debug.h"

/*
 * As in previous hws the main function must be in its own file!
 */



int main(int argc, char const *argv[], char* envp[]){
    /* DO NOT MODIFY THIS. If you do you will get a ZERO. */
    rl_catch_signals = 0;
    /* This is disable readline's default signal handlers, since you are going to install your own.*/
    Signal(SIGALRM,alarmHandler);
    //Disable SIGTSTP
    Signal(SIGTSTP, SIG_IGN);
    //Signal handler for SIGUSR2
    Signal(SIGUSR2, userHandler);
    //Signal handler for SIGCHLD
    struct sigaction childAction;
    childAction.sa_sigaction = childHandler;
    //sigfillset(&childAction.sa_mask);
    childAction.sa_flags = SA_SIGINFO;
    sigaction(SIGCHLD, &childAction, NULL);
    char* cwd = getcwd(NULL, MAX_SIZE);
    const char* myName = "<jonyin> : <";
    char nameBuffer[1024];
    //Start with the empty string.
    nameBuffer[0] = '\0';
    strcat(nameBuffer,myName);
    strcat(nameBuffer, cwd);
    strcat(nameBuffer, "> $");
    char *cmd = readline(nameBuffer);

    while(cmd != NULL && cwd != NULL) {
        char** buf = NULL;
        char** argArray = tokenize(cmd, " \t");
        int test = getBuiltInCommand(argArray[0]);
        //int testResult = containsChar(argArray, '<') || containsChar(argArray, '>');
        //printf("The result of the test is %d\n",testResult);
        switch(test)
        {
            case 0:
            {
                free(cmd);
                free(buf);
                free(cwd);
                exit(EXIT_SUCCESS);
            }
            case 1:
            {
                //int child_status = 0;

                printHelp(NULL);

                break;
            }
            case 2:
            {
                int status = changeDirectory(argArray[1]);
                if (status != 0)
                {
                    fprintf(stderr, "Change Directory Error: %s\n", strerror(errno));
                }
            }
            case 3:
            {
                //Check to see if the piping is valid
                if (checkIfValid(argArray))
                {
                    //Break if the re exists a pipe and redirect, this is not valid.
                    break;
                }
                else if (containsChar(argArray, '|'))
                {
                    //Piping command There can either be 2 or 3 piping commands
                    char** commands[3];
                    commands[0] = malloc(0);
                    commands[1] = malloc(0);
                    commands[2] = malloc(0);
                    int pipes = countPipes(argArray);
                    for (int i = 0; i < pipes + 1; i++)
                    {
                        commands[i] = extractPipe(argArray, i);
                    }
                    //printf("Executing a program");
                    if (pipes == 1)
                    {
                        int fd1[2];
                        int stin = dup(0);
                        //pipe it
                        pipe(fd1);
                        //Execute two programs therefore fork once.
                        if (fork() == 0)
                        {
                            //Execute first program, feed it to Pipe
                            //close pipe read part
                            close(fd1[0]);
                            //Overwrite stdout with the input of the pipe.
                            dup2(fd1[1], 1);
                            //close pipe
                            close(fd1[1]);
                            //execute program.
                            if (execute(commands[0]) != 0)
                                exit(EXIT_SUCCESS);
                            exit(EXIT_FAILURE);
                        }
                        else
                        {
                            //parent executes the second program.
                            //Close portion to write to pipe.
                            close(fd1[1]);
                            //Replace stdin with the output of the pipe.
                            dup2(fd1[0],0);
                            //close pipe.
                            close(fd1[0]);
                            execute(commands[1]);
                            int status;
                            wait(&status);
                            //Restore stdin.
                            dup2(stin, 0);
                            if (WIFEXITED(status))
                            {
                                if (WEXITSTATUS(status) != EXIT_SUCCESS)
                                {
                                    freeArray(commands);
                                    break;
                                }
                            }
                            else
                            {
                                freeArray(commands);
                                break;
                            }
                        }

                    }
                    else if (pipes == 2)
                    {
                        int fd1[2];
                        int fd2[2];
                        int stin = dup(0);
                        pipe(fd1);
                        pipe(fd2);
                        if (fork() == 0)
                        {
                            if (fork() == 0)
                            {
                                //Run first pipe and program.
                                //Close the read portion.
                                close(fd1[0]);
                                close(fd2[0]);
                                close(fd2[1]);
                                dup2(fd1[1],1);
                                close(fd1[1]);
                            if (execute(commands[0]) != 0)
                                exit(EXIT_SUCCESS);
                            exit(EXIT_FAILURE);
                            }
                            else
                            {
                                //Link this pipe to the second pipe.
                                close(fd1[1]);
                                close(fd2[0]);
                                dup2(fd1[0], 0);
                                dup2(fd2[1], 1);
                                close(fd1[0]);
                                close(fd2[1]);
                                if (execute(commands[1]) != 0)
                                    exit(EXIT_SUCCESS);
                                exit(EXIT_FAILURE);
                            }

                        }
                        else
                        {
                            //Parent executes the third program.
                            close(fd1[0]);
                            close(fd1[1]);
                            close(fd2[1]);
                            dup2(fd2[0], 0);
                            if (execute(commands[2]) != 0)
                            {
                                freeArray(commands);
                                break;
                            }
                            dup2(stin,0);
                            int status;
                            wait(&status);
                            if (WIFEXITED(status))
                            {
                                if (WEXITSTATUS(status) == EXIT_FAILURE)
                                {
                                    freeArray(commands);
                                    break;
                                }
                            }
                        }
                    }
                    else if (pipes == 3)
                    {
                        for (int i = 0; i < pipes + 1; i++)
                        {
                            free(commands[i]);
                        }
                        break;
                    }

                    for (int i = 0; i < pipes + 1; i++)
                    {
                        //printf("GOT THIS FAR");
                        free(commands[i]);
                    }
                    break;
                }
                else if (containsChar(argArray, '<') || containsChar(argArray, '>'))
                {
                    //Redirect command.
                    int infd = dup(0);
                    int outfd = dup(1);
                    char* strings[2];
                    //NEW INPUT
                    strings[0] = NULL;
                    //NEW OUTPUT
                    strings[1] = NULL;
                    char** newArray = constructArgv(argArray, strings);
                    if (newArray == NULL){
                        free(newArray);
                        break;
                    }
                    //argsLength(newArray, 0);
                    //printf("STRING1: %s\n",strings[0]);
                    //printf("STRING2: %s\n",strings[1]);
                    int rdfd = -1;
                    int wrfd = -1;
                    if (strings[0] != NULL)
                    {
                        if ((rdfd = open(strings[0], O_RDONLY)) != -1)
                        {
                            //Valid input specified. Now redirect stdin(0) to wrfd.
                            dup2(rdfd, 0);
                        }
                        else
                        {
                            //Invalid input for file name.
                            free(newArray);
                            printf("INVALID FILENAME SPECIFIED");
                            break;
                        }
                    }
                    if (strings[1] != NULL)
                    {
                        if ((wrfd = open(strings[1], O_CREAT|O_TRUNC|O_WRONLY, 0666)) != -1)
                        {
                            //Valid input specified. Now redirect stdin(0) to wrfd.
                            //printf("\nWriting has been successfully duped");
                            dup2(wrfd, 1);
                        }
                        else
                        {
                            //Invalid input for file name.
                            free(newArray);
                            printf("PROBLEM WITH WRITING FILE");
                            break;
                        }
                    }
                    execute(newArray);
                    if (rdfd != -1)
                    {
                        close(rdfd);
                    }
                    if (wrfd != -1)
                    {
                        close(wrfd);
                    }
                    free(newArray);
                    dup2(infd, 0);
                    dup2(outfd, 1);
                    break;
                }
                else
                {
                    if(execute(argArray) != 0)
                        printf("EXECUTE ERROR");
                    break;
                }
            }
            case 4:
            {
                //Alarm
                setAlarm(argArray);
            }
            default:
            {
            }

        }
        //printf("%s\n",copy);
        /* All your debug print statements should use the macros found in debu.h */
        /* Use the `make debug` target in the makefile to run with these enabled. */
        //info("Length of command entered: %ld\n", strlen(copy));
        // Free copied string
        //printf("DOES IT GET HERE?");
        free(cwd);
        free(cmd);
        free(buf);
        free(argArray);
        cwd = getcwd(NULL, MAX_SIZE);
        nameBuffer[0] = '\0';
        strcat(nameBuffer,myName);
        strcat(nameBuffer, cwd);
        strcat(nameBuffer, "> $");
        //printf("\nTEST CWD: %s", cwd);
        cmd = readline(nameBuffer);
        /* You WILL lose points if your shell prints out garbage values. */
    }

    /* Don't forget to free allocated memory, and close file descriptors. */
    free(cmd);
    return EXIT_SUCCESS;
}
