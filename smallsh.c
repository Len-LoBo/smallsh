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

void main()
{
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
        prompt(); 
        userInput = getUserInput();
        token = strtok(userInput, " \n");
	char* args[512];
    
	//checks for built in commands
        if (strcmp(token, exitText) == 0)
        {
            exitShell();
        }
	else if (strcmp(token, changeDirText) == 0)
	{
	    token = strtok(NULL, " \n");
            changeDir(token);
 	}
	else if (strcmp(token, statusText) == 0)
	{
	    getStatus();
	}

	//non built in command attempted
	else
	{

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



