#include "sfish.h"
/**
* Wrapper function for the signal function in order to allow more consistent behavior.
**/

volatile sig_atomic_t Timer, duration, num_bytes = 0;

int countChars(char** argV, char* word)
{
	char** copy = argV;
	int count = 0;
	for ( ; (*copy) != NULL; copy++)
	{
		if (strcmp(*argV, word) == 0)
			count++;
	}
	return count;
}


/**
* Checks whether if the following
**/
int checkIfValid(char** argArray)
{
	if ((containsChar(argArray, '|') && (containsChar(argArray, '<') || containsChar(argArray, '>'))))
	{
		return 0;
	}
	if ((countChars(argArray, "<")) > 1 || (countChars(argArray, ">")) > 1)
	{
		return 0;
	}
	if ((countChars(argArray, "<")) == 1 || (countChars(argArray, ">")) == 1)
	{
		return 1;
	}
	else if (countChars(argArray, "|") == 1 || countChars(argArray, "|") == 2)
	{
		return 1;
	}
	return 0;
}

handler_t* Signal(int signum, handler_t* handler)
{
	struct sigaction old,new;
	new.sa_handler = handler;
	sigemptyset(&new.sa_mask);
	new.sa_flags = SA_RESTART;
	if (sigaction(signum,&new,&old)< 0)
		exit(0);
	return (old.sa_handler);
}

void alarmHandler(int sig)
{
	sigset_t new,old;
	sigfillset(&new);
	sigprocmask(SIG_BLOCK, &new, &old);
	//BLOCK ALL SIGNALS,
	write(1, "Your ", 5);
	char length[num_bytes];
	sprintf(length,"%d" ,duration);
	write(1, length, num_bytes);
	write(1, " ", 1);
	write(1, "second timer has finished!", 26);
	sigprocmask(SIG_SETMASK,&old, NULL);
	Timer = 0;
	num_bytes = 0;
}

void childHandler(int sig, siginfo_t * info, void* ignore)
{
	// Block all incoming signals until this is finished
	printf("Child with PID $%d has died. It spent %ld milliseconds using the CPU", info->si_pid, info -> si_stime);
}

void userHandler(int sig)
{
	write(1,"Well that was easy.", 20);
}

int isNumber(char* string)
{
	for (; *string != '\0'; string++)
	{
		if (!isdigit(*string))
			return 0;
	}
	return 1;
}

/**
* Test method to see if the program correctly tokenizes a line of input.
**/
void testMethod(char** arrayStrings)
{
    char* currString = arrayStrings[0];
    int currIndex = 1;
    while (currString != NULL)
    {
        printf("%s\n",currString);
        currString = arrayStrings[currIndex];
        currIndex++;
    }
    printf("NULL\n");
}

int setAlarm(char** argV)
{
	if (argV[1] == NULL || !isNumber(argV[1]) ||  Timer == 1)
	{
		exit(EXIT_FAILURE);
	}
	else
	{
		int num = 0;
		//printf("Setting alarm!");
		num_bytes = strlen(argV[1]);
		char* endPtr = argV[1] + num_bytes - 1;
		num = strtol(argV[1], &endPtr, 10);
		Timer = 1;
		duration = num;
		alarm(num);
	}
	return EXIT_SUCCESS;
}

/**
*This method is used to determine what built in command to execute.
**/
int getBuiltInCommand(char* commandName)
{
	if (commandName == NULL)
	{
		return 5;
	}
    else if (strcmp(commandName, "exit")==0)
    {
        return 0;
    }
    else if (strcmp(commandName, "help")==0)
    {
        return 1;
    }
    else if (strcmp(commandName, "cd")==0)
    {
        return 2;
    }
    else if(strcmp(commandName, "pwd")==0)
    {
        return 3;
    }
    else if (strcmp(commandName,"alarm") == 0)
    {
    	return 4;
    }
    return 3;
}

int containsChar(char** argv,char character)
{
	int index = 0;
	while (argv[index] != NULL)
	{

		if (*argv[index] == character)
		{
			return 1;
		}
		index++;
	}
	return 0;
}

void freeArray(char*** array)
{
	for (int i = 0; i < 3; i++)
	{
		free(array[i]);
	}
}

/**
* This method tokenizes a line of input and strips off extraneous white spaces. Returns an array of tokens.
**/
char** tokenize(char* cmd, char* delims)
{
    char* token = strtok(cmd, delims);
    char** buffer = NULL;
    if (token == NULL)
    {
        buffer = (char**)(malloc(sizeof (NULL)));
        buffer[0] = NULL;
        return buffer;
    }
    else
    {
        int totalSize = 0;
        buffer = (char**)malloc(sizeof(token));
        buffer[0] = token;
        totalSize = sizeof(token);
        int nextIndex = 1;
        while (token != NULL)
        {
            token = strtok(NULL,delims);
            buffer = (char**)realloc(buffer, sizeof(token) + totalSize);
            totalSize += sizeof(token);
            buffer[nextIndex] = token;
            nextIndex++;
        }
        buffer = (char**)(realloc(buffer,sizeof (NULL) + totalSize));
        buffer[nextIndex] = NULL;
        return buffer;
    }
}

void printWorkingDirectory()
{
	char* path = getcwd(NULL, MAX_SIZE);
	fprintf(stdout, "%s\n", path);
	free(path);
	exit(EXIT_SUCCESS);
}

void printHelp(char* pathName)
{
	char* helpString = (char*) malloc(MAX_SIZE);
	if (pathName == NULL)
	{
		fprintf(stdout, "%-50.50s%-50.50s\n%-50.50s%-50.50s\n%-50.50s%-50.50s\n%-50.50s%-50.50s\n", "Help:", "Displays all built in commands",
			"cd [dir]:", "Changes the current directory to the one specified in dir",
			"pwd:", "Outputs the current working directory",
			"exit:", "Quits the shell");
	}

	free(helpString);
	exit(EXIT_SUCCESS);
}

int changeDirectory(char* pathName)
{
	if (pathName == NULL)
	{
		char* oldDirectory = getcwd(NULL,MAX_SIZE);
		setenv("OLDPWD", oldDirectory, 1);
		free(oldDirectory);
		char* homeDirectory = getenv("HOME");
		chdir(homeDirectory);
		char* cwd = getcwd(NULL, MAX_SIZE);
		setenv("PWD", cwd, 1);
		free(cwd);
		return 0;
	}
	else if (strcmp(pathName, "-") == 0)
	{
		char* lastDirectory = getenv("OLDPWD");
		char* oldDirectory = getcwd(NULL,MAX_SIZE);
		setenv("OLDPWD", oldDirectory, 1);
		free(oldDirectory);
		chdir(lastDirectory);
		setenv("PWD", lastDirectory, 1);
		return 0;
	}
	else
	{
		if (chdir(pathName) == 0)
		{
			char* cwd = getcwd(NULL, MAX_SIZE);
			setenv("PWD", cwd, 1);
			free(cwd);
			return 0;
		}
		else
			return 1;
	}
}

int argsLength(char** array, int countPipe)
{
	int index = 0;
	if (countPipe)
	{
		while (array[index] != NULL && strcmp(array[index], "|") != 0)
		{
			index++;
		}
	}
	else
	{
		while (array[index] != NULL)
		{
			index++;
		}
	}
	return index;
}

//Returns an argv which strips away redirection symbols and pipes, leaving behind only arg values
//Returns NULL for errors.
char** constructArgv(char** oldargV, char** fileNames)
{
	char** outputBuffer = malloc(0);
	int totalSize = 0;
	int newindex = 0;
	int argsSize = argsLength(oldargV,0);
	//printf("%d",argsSize);
	for (int i = 0; i < argsSize; i++)
	{
		if (strcmp(oldargV[i], "<") == 0)
		{
			if (i == argsSize)
			{
				//Cannot go any further
				return NULL;
			}
			else
			{
				fileNames[0] = oldargV[i+1];
				i++;
			}
		}
		else if(strcmp(oldargV[i],">") == 0)
		{
			if (i == argsSize)
			{
				//Cannot go any further
				return NULL;
			}
			else
			{
				fileNames[1] = oldargV[i+1];
				i++;
			}
		}
		else
		{
			totalSize += sizeof(oldargV[i]);
			outputBuffer = (char**)(realloc(outputBuffer, totalSize + sizeof(oldargV[i])));
			outputBuffer[newindex] = oldargV[i];
			newindex++;
		}
	}
	totalSize += sizeof(NULL);
	outputBuffer = (char**)(realloc(outputBuffer, totalSize));
	outputBuffer[newindex] = NULL;
	return outputBuffer;
}


int countPipes(char** argV)
{
	int index = 0;
	int count = 0;
	while (argV[index] != NULL)
	{
		if (strcmp(argV[index], "|") == 0)
			count++;
		index++;
	}
	return count;
}

char** extractPipe(char** argv, int pipeNo)
{
	//printf("Extracting Pipe");
	int pipesSeen = 0;
	char ** array = malloc(0);
	int index = 0;
	int totalSize = 0;
	int arrayInd = 0;
	while (argv[index] != NULL && pipesSeen != pipeNo)
	{
		if (strcmp(argv[index], "|") == 0)
		{
			pipesSeen++;
		}
		index++;
	}
	//printf("Checkpoint 1");
	//Array now points to beginning of prog arguments.
	while ( argv[index] != NULL && (strcmp(argv[index], "|") != 0 ))
	{
		//printf("Checkpoint 2");
		//printf("TEST: %d\n", argv[index] == NULL);
		totalSize += sizeof(argv[index]);
		array = realloc(array,totalSize);
		array[arrayInd] = argv[index];
		arrayInd++;
		index++;
	}
	array = realloc(array, totalSize + sizeof(NULL));
	array[arrayInd] = NULL;
	return array;
}


/**
* This method will execute a program using a specified argv array.
**/

int execute(char** argArray)
{
	if (strcmp(argArray[0], "pwd") == 0)
	{
		//Issued the print working directory command
		if (fork() == 0)
		{
			printWorkingDirectory();
		}
		else
		{
			int status;
			wait(&status);
			return 0;
		}
	}
	char* index = strchr(argArray[0],'/');
	if (index != NULL)
	{
                    //Check status of file, is it regular?
		struct stat stats;
		int success = stat(argArray[0], &stats);
		if (success == -1)
		{
                    //Failure to generate stats, break out of case, don't execute
			printf("Command doesn't exist");
			return EXIT_FAILURE;
		}
		else
		{
			if (S_ISREG(stats.st_mode))
			{
                            //Attempt to execute the file.
				//int child_status;
				if (fork() == 0)
				{
					if (execv(argArray[0],argArray) == -1)
					{
                                    //If an error occurs, exit out of the child.
						printf("Failure to run specified program");
						exit(EXIT_FAILURE);
					}
				}
				else
				{
					int child_status;
                    //Wait for child to complete
					//
					wait(&child_status);
					if (WIFEXITED(child_status))
					{
						return WEXITSTATUS(child_status);
					}
					return 1;
				}
			}
			else
			{
                //Failure to point at a regular file, don't execute
				printf("This is not a valid file to execute");
				return EXIT_FAILURE;
			}
		}

	}
	else
	{
                    //Check through the PATH variable
		struct stat stats;
		char copy[MAX_SIZE];
		strcpy(copy, getenv("PATH"));
		char** pathNames = tokenize(copy, ":");
		char pathBuffer[MAX_SIZE];
		int index = 0;
		char* path = NULL;
		while (pathNames[index] != NULL)
		{
			strcpy(pathBuffer, pathNames[index]);
			strcat(pathBuffer, "/");
			strcat(pathBuffer, argArray[0]);
                        //printf("Proposed path %d: %s\n", index, pathBuffer);
			if (stat(pathBuffer, &stats) == 0)
			{
                            //Success Check if file is regular file.
				if (S_ISREG(stats.st_mode))
				{
                     //If so, set the path to non-null, break
					path = pathBuffer;
					break;
				}
			}
                        //No success with finding a file here, try next index.
			index++;
		}
		if (path != NULL)
		{

			//int child_status;
			if (fork() == 0)
			{
				if (execv(path,argArray) == -1)
				{
					printf("Error with launching program from path");
                                    //If an error occurs, exit out of the child.
					exit(EXIT_FAILURE);
				}
			}
			else
			{
                                //Wait for child to complete
				int child_status;
				wait(&child_status);
				if (WIFEXITED(child_status))
				{
					return WEXITSTATUS(child_status);
				}
					return 1;

			}
		}
		else
		{
			printf("Cannot find program");
			return EXIT_FAILURE;
		}
		free(pathNames);

	}
	return 0;
}


