#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

void prompt();
char* getUserInput();
void exitShell();
void changeDir(char*);
void getStatus();
void execute(char**);

void main()
{
    pid_t spawnPid = -5;
    int childExitMethod = -5;

    // string for storing user input and tokenizing
    char* userInput;
    char* token;

    //built in commands
    char* exitText = "exit";
    char* changeDirText = "cd";
    char*statusText = "status";

    //prints prompt to screen
    while (1)
    {
        	
	char* args[513];
	int argCounter = 0;

	//initializes args array pointers to NULL
	for (int i = 0; i < 513; i++)
	{
	    args[i] = NULL;
	}
	
	//prompts user for command line input
        prompt(); 

	//stores user input
        userInput = getUserInput();
        
	//tokenizes input to get commands, stores in array position 0
        token = strtok(userInput, " \n");	
	args[argCounter] = token;
        argCounter++;	
	
	//stores remaining arguments in argument array
	while ((token = strtok(NULL, " \n")) != NULL)
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
	    getStatus();
	}
	else 
	{
	    //fork() child 
	    spawnPid = fork();
	    //if fork fails -- ERROR
	    switch (spawnPid)
	    {
	        case -1: { perror("Process fork() failed!\n"); }	
	        case 0: {
	                //printf("Child process created.\n"); 
			execute(args);
			perror("Exec failure");
	        }
	        default: {
	                //waits for child process to terminate
	                waitpid(spawnPid, &childExitMethod, 0);
	        }
	    }
	}
    }
}

//prompt() --  displays the command line prompt
void prompt()
{
    printf(": ");
    fflush(stdout);
}

//getUserInput() -- gets user input from stdin, and returns a string
char* getUserInput()
{

    char* inputBuffer = NULL;
    int numChars = -1;
    size_t bufferSize = 0;

    numChars = getline(&inputBuffer, &bufferSize, stdin);
    if (numChars == -1)
    {
        clearerr(stdin);
    }

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
void getStatus()
{
    printf("getstatus");
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
