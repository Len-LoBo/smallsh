#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>
#include "dynArray.h"

char* prompt();
void exitShell();
void changeDir(char*);
void getStatus(int);
void execute(char**);
char* replacePid(char*);
void catchIgn();

void main()
{
    pid_t spawnPid = -5;
    int childExitMethod = -5;

    //signal handlers
    struct sigaction INT_action = {0}, SIGSTP_action = {0};

    //sig int
    INT_action.sa_handler = SIG_IGN; 
    sigfillset(&INT_action.sa_mask);
    INT_action.sa_flags = 0;
    sigaction(SIGINT, &INT_action, NULL);

    //sig stp
    SIGSTP_action.sa_handler = catchSTP;
    sigfillset(&SIGSTOP_action.sa_mask);
    SIGSTOP_action.sa_flags = 0;
    
 
    DynArr* pidArray = newDynArr(5);

    // string for storing user input and tokenizing
    char* userInput;
    int lastCharIndex;
    char* token;

    //built in commands
    char* exitText = "exit";
    char* changeDirText = "cd";
    char*statusText = "status";

    //prints prompt to screen
    while (1)
    {
	pid_t childPid;
        	
	char* args[513];
	int argCounter = 0;
	
	//initializes args array pointers to NULL
	for (int i = 0; i < 513; i++)
	{
	    args[i] = NULL;
	}
       
	for (int i = 0; i < sizeDynArr(pidArray); i++)
	{
	    if ((childPid = waitpid(getDynArr(pidArray, i), &childExitMethod, WNOHANG)) > 0) 
	    {
                removeAtDynArr(pidArray, i);
		printf("background pid %d is done: ", childPid);
		getStatus(childExitMethod);
	    }	    
	}
	//
	//stores user input
        userInput = prompt();
        userInput = replacePid(userInput);
    

	if (strcmp(userInput, "") == 0) continue;
        lastCharIndex = strlen(userInput) - 2; 

	//handles COMMENTS
	if (*userInput == '#')
	    continue;
        
	//tokenizes input to get commands, stores in array position 0
        token = strtok(userInput, " \n");	
	args[argCounter] = token;
        argCounter++;	
	
	//stores remaining arguments in argument array
	while ((token = strtok(NULL, " \n")) != NULL 
			&& strcmp(token, "<") != 0 
			&& strcmp(token, ">") != 0
			&& strcmp(token, "&") != 0) 
	{ 
            args[argCounter] = token;
	    argCounter++;
	}
        

	//checks for built in commands
        if (strcmp(*args, exitText) == 0)
        {
            exitShell();
        }
	else if (strcmp(*args, changeDirText) == 0)
	{
	    if (args[1] == NULL) changeDir(getenv("HOME"));
	    else changeDir(args[1]);
 	}
	else if (strcmp(*args, statusText) == 0)
	{
	    getStatus(childExitMethod);
	}
	// if not build in command
	else 
	{
	    //fork() child 
	    spawnPid = fork();
	    //if fork fails -- ERROR
	    switch (spawnPid)
	    {
	        case -1: { perror("Process fork() failed!\n"); }	
	        case 0: {
                        INT_action.sa_handler = SIG_DFL;
                        sigaction(SIGINT, &INT_action, NULL);

			//perform redirection
			//redirection for BG process with no user specified redirection
			if (userInput[lastCharIndex] == '&')
			{
			    INT_action.sa_handler = SIG_IGN;
			    sigaction(SIGINT, &INT_action, NULL);
			    //redirect stdout
			    int targetFD = open("/dev/null", O_WRONLY, 0644);
			    if (targetFD == -1)
			    {
			        perror("open()");
				exit(1);
			    }
			    else 
			    {
			        int result = dup2(targetFD, 1);
				if (result == -1)
				{
			            perror("dup2_stdout");
				    exit(2);
				}
			    }
			    //redirect stdin
			    int sourceFD = open("/dev/null", O_RDONLY);

	                    if (sourceFD == -1) 
			    { 
			        perror("open()"); 
			        exit(1);
			    }
				
			    else 
			    {
			        int result = dup2(sourceFD, 0);
			        if (result == -1) 
				{ 
			            perror("dup2_stdin"); 
			            exit(2);
				}	
		            }	

			}

			//perform user input reditrection
			while (token != NULL)
			{
                            //checks for stdout redirect
		            if (strcmp(token, ">") == 0)
			    {
		                token = strtok(NULL, " \n");
				token = replacePid(token);

				int targetFD = open(token, O_WRONLY | O_CREAT | O_TRUNC, 0644);

				if (targetFD == -1) 
				{
				    perror("open()");
			            exit(1); 
				}
				else
				{
				    int result = dup2(targetFD, 1);
				    if (result == -1) 
				    { 
			                perror("dup2_stdout"); 
					exit(2);
				    }
				    
				}
			    }
			    //checks for stdin redirect
		            else if (strcmp(token, "<") == 0)	
			    {
		                token = strtok(NULL, " \n");
				token = replacePid(token);

				int sourceFD = open(token, O_RDONLY);

				if (sourceFD == -1) { 
				    perror("open()"); 
				    exit(1);
				}
				
				else 
			        {
			            int result = dup2(sourceFD, 0);
			            if (result == -1) { 
				        perror("dup2_stdin"); 
					exit(2);
				    }	
				}
			    }
			    //looks for more operators/commands
			    token = strtok(NULL, " \n");
			}
			
	                //printf("Child process created.\n"); 
			execute(args);
			perror("Exec failure");
	        }
	        default: 
		{
	                //BG Process
		        if (userInput[lastCharIndex] == '&')
			{
			    addDynArr(pidArray, spawnPid);
			    //print pid of child process
			    printf("background pid is %d\n", spawnPid); 

			}
			//FG Process
			else
			{
	                    //waits for child process to terminate
	                    childPid = waitpid(spawnPid, &childExitMethod, 0);
                            if (childPid == -1)
			    {
			        perror("wait failed");
			    }
			}
	        }
	    }
	}
    }
}


//getUserInput() -- gets user input from stdin, and returns a string
char* prompt()
{
    int numChars = -1;
    char* inputBuffer; 
    do
    {
        printf(": ");
        fflush(stdout);

        size_t bufferSize = 0;
        inputBuffer = NULL;

        numChars = getline(&inputBuffer, &bufferSize, stdin);
	
    } while (numChars < 2);

    return inputBuffer; 
}


//exits smallsh
void exitShell()
{
    exit(0);
}


// changeDir() -- changes the current working directory
void changeDir(char* path)
{
    int result = chdir(path);
    char dirBuffer[255];

    if (result == -1)
    {
        perror("Error");
    }
    else 
    {
	getcwd(dirBuffer, sizeof(dirBuffer));
        printf("Working Directory Change To: %s\n", dirBuffer); 
    }
}


//getStatus() -- returns exit code or signel number of most recently terminated child process
void getStatus(int childExitMethod)
{
    int exitStatus;
    //check if child process terminated normally
    if (WIFEXITED(childExitMethod))
    {
        exitStatus = WEXITSTATUS(childExitMethod);
	printf("exit value %d\n", exitStatus);
        fflush(stdout);

    }
    //checks for signal termination of child
    if (WIFSIGNALED(childExitMethod))
    {
        exitStatus = WTERMSIG(childExitMethod);
        printf("terminated by signal %d\n", exitStatus);
	fflush(stdout);
    }
}

// execute() -- executes a program using execvp, prints error if exec fails
void execute(char** argv)
{
    if (execvp(*argv, argv) < 0)
    {
        perror("Command Exec failure!");
	exit(1);
    }
}

//variable expansion for $$
char* replacePid(char* text)
{
    pid_t currentPid = getpid();
    int pid = (int) currentPid;

    static char buffer[4096];
    char *location;
    if (!(location = strstr(text, "$$")))
        return text;

    strncpy(buffer, text, location - text);
    buffer[location - text] = '\0';
    sprintf(buffer + (location - text), "%d%s", pid, location + 2);

    return buffer;

}
