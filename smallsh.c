// Program Name: smallsh.c
// Last Modified: 2/20/20
// Author: Leonard LoBiondo
// Description:
//            Program implements a shell with a limited set of built commands.  
//
//            Non-built in commands are run using exec().  
//            Program uses fork() to create child processes when non built-in commands are run.  
//            Programs can be run in the BG by ending a command with '&', and lines
//            beginning with '#' will be ignored by the shell as comments.  
//
//            TERM signals will only terminated foreground processes, and sending 
//            a SIGTSTP signal will enter FG only mode, where BG processes are disabled.  
//
//            Exit the shell by typing the "exit" command. 
//
//            Note: DynArr is an implementation of a dynamic array, written by the author for CS261.
//
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>
#include "dynArray.h"  //dynamic array

//Function declarations
void exitShell();
void changeDir(char*);
void getStatus(int);
void execute(char**);
char* replacePid(char*);
void prompt();
char* getUserInput();
void reapBackground(DynArr*);
void catchSTP();

//GLOBAL VARIABLE
int stopped = 1;    // tracks whether FG mode is activated

//MAIN
void main()
{
    //holds process ids
    pid_t spawnPid = -5;

    //holds exit method for fg processes
    int childExitMethod = -5;

    //signal handlers
    struct sigaction INT_action = {0}, SIGSTP_action = {0};

    //SIGINT signal handler
    //Parent ignores
    INT_action.sa_handler = SIG_IGN; 
    sigfillset(&INT_action.sa_mask);
    INT_action.sa_flags = 0;
    sigaction(SIGINT, &INT_action, NULL);

    //SIGTSTP signal handler
    //caught by parent and calls catchSTP
    SIGSTP_action.sa_handler = catchSTP;
    sigfillset(&SIGSTP_action.sa_mask);
    SIGSTP_action.sa_flags = 0;
    sigaction(SIGTSTP, &SIGSTP_action, NULL);
    
    //dynamic array for storing non-reaped BG pids
    DynArr* pidArray = newDynArr(5);

    //strings for storing user input and tokenizing
    char* userInput;
    char* token;

    //stores last character of userInput, used for identifying BG processes '&'
    int lastCharIndex;

    //built in commands
    char* exitText = "exit";
    char* changeDirText = "cd";
    char*statusText = "status";

    
    // return value for parent waitpid -- reaping FG processes
    pid_t childPid;
    
    // COMMAND LINE LOOP
    while (1)
    {
	//BG PROCESS REAPING---------------------------------    
	//reaps terminated background process in pidArray
	
        reapBackground(pidArray);

	//---------------------------------------------------
        

	//COMMAND ARGUMENT ARRAY ---------------------------------------
	// array for storing arguments passed to exec()
	char* args[513];
	int argCounter = 0;
	
	//initializes args array pointers to NULL
	for (int i = 0; i < 513; i++)
	{
	    args[i] = NULL;
	}
	// -------------------------------------------------
       
	
        //USER INPUT ----------------------------------------------------
	//Displays command prompt and gets input from user via stdin
	userInput = getUserInput();

	//expands $$ to current pid
        userInput = replacePid(userInput);
    
        //if user enters a blank string (presses only enter), program returns to top of loop
	//Result: user is reprompted
	if (strcmp(userInput, "") == 0) continue;
        lastCharIndex = strlen(userInput) - 2;   // -2 takes into account \n 

	//COMMENTS - ignored by shell
	// if first character is #
	if (*userInput == '#')
	    continue;

	// END USER INPUT ----------------------------------------------
        
	//INPUT PARSING------------------------------------------------
	
	//tokenizes  first input element -- command
        token = strtok(userInput, " \n");	
	args[argCounter] = token;
	argCounter++;

	//tokenizes until it hits a redirection symbol, BG symbol, or end of input
	//stores command and all arguments in args array
	while ((token = strtok(NULL, " \n")) != NULL 
			&& strcmp(token, "<") != 0 
			&& strcmp(token, ">") != 0
			&& strcmp(token, "&") != 0) 
	{ 
            args[argCounter] = token;
	    argCounter++;
	}
        

	//BUILT-IN COMMANDS (FG ONLY)---------------------------------------
        if (strcmp(*args, exitText) == 0)
        {
            //exits smallsh
            exitShell(pidArray);
        }
	else if (strcmp(*args, changeDirText) == 0)
	{
	    //changes directory
	    if (args[1] == NULL) changeDir(getenv("HOME"));
	    else changeDir(args[1]);
 	}
	else if (strcmp(*args, statusText) == 0)
	{
            //returns exit status or terminating signal of last child
	    getStatus(childExitMethod);
	}

	//NOT BUILT-IN COMMAND--------------------------------------------
	else 
	{
	    //fork() child 
	    spawnPid = fork();
	    switch (spawnPid)
	    {
	        //if fork fails -- ERROR
	        case -1: { perror("Process fork() failed!\n"); }	
	        
                //CHILD PROCESS --------------------------------------------------
		case 0: {

                        //sets SIGINT to default action (termination)
                        INT_action.sa_handler = SIG_DFL;
                        sigaction(SIGINT, &INT_action, NULL);

			// I/O REDIRECTION -------------------------------------------------
			//redirection for BG process with no user specified redirection
			if (userInput[lastCharIndex] == '&' && stopped == 1)
			{
			    //sets SIGINT to ignore for BG children
			    INT_action.sa_handler = SIG_IGN;
			    sigaction(SIGINT, &INT_action, NULL);

			    //REDIRECT STDOUT to /dev/null -------------------------------------

			    int targetFD = open("/dev/null", O_WRONLY, 0644);
			    
			    //open() FAIL error
			    if (targetFD == -1)
			    {
			        perror("open()");
				exit(1);
			    }

			    //open SUCCESS
			    else 
			    {
				//redirects stdout to target
				//ERROR on dup() failure    
			        int result = dup2(targetFD, 1);
				if (result == -1)
				{
			            perror("dup2_stdout");
				    exit(2);
				}
			    }

			    //REDIRECT STDIN to /dev/null ----------------------------------------
			    int sourceFD = open("/dev/null", O_RDONLY);

			    //open() FAIL
	                    if (sourceFD == -1) 
			    { 
			        perror("open()"); 
			        exit(1);
			    }
			    
			    //open() SUCCESS
			    else 
			    {
				//redirects stdin to target    
				// ERROR on dup() failure
			        int result = dup2(sourceFD, 0);
			        if (result == -1) 
				{ 
			            perror("dup2_stdin"); 
			            exit(2);
				}	
		            }	

			}

			//USER INPUT REDIRECTION -----------------------------------------------
			//perform user input reditrection
			while (token != NULL)
			{
			    //STDOUT
			    //	
                            //checks for stdout redirect
		            if (strcmp(token, ">") == 0)
			    {
				//gets redirect target 
		                token = strtok(NULL, " \n");
                                
				//opens target for output
				int targetFD = open(token, O_WRONLY | O_CREAT | O_TRUNC, 0644);

				//error on open() FAIL
				if (targetFD == -1) 
				{
				    perror("open()");
			            exit(1); 
				}

				//OPEN SUCCESS
				else
				{
				    //redirects stdout to target	
				    int result = dup2(targetFD, 1);
				    if (result == -1) 
				    { 
			                perror("dup2_stdout"); 
					exit(2);
				    }
				    
				}
			    }

			    //STDIN
			    //
			    //checks for stdin redirect
		            else if (strcmp(token, "<") == 0)	
			    {
				//gets redirect source    
		                token = strtok(NULL, " \n");

				//opens source for input
				int sourceFD = open(token, O_RDONLY);

				//error on open() FAIL
				if (sourceFD == -1) { 
				    perror("open()"); 
				    exit(1);
				}
				
				//OPEN SUCCESS
				else 
			        {
				    //redirects source to stdin
			            int result = dup2(sourceFD, 0);
			            if (result == -1) { 
				        perror("dup2_stdin"); 
					exit(2);
				    }	
				}
			    }

			    //looks for more redirection operators and targets
			    token = strtok(NULL, " \n");
			}
			
			//EXECUTES COMMAND with args 
			execute(args);

			//if this is reached, exec failed
			perror("Exec failure");
	        }

		//PARENT PROCESS -----------------------------------------------------	 
	        default: 
		{
	                //BG Process
		        if (userInput[lastCharIndex] == '&' && stopped == 1)
			{
			    //adds pid to BG pid array	
			    addDynArr(pidArray, spawnPid);

			    //print pid of BG child process
			    printf("background pid is %d\n", spawnPid); 

			}
			//FG Process
			else
			{
	                    //waits for child process to terminate
	                    childPid = waitpid(spawnPid, &childExitMethod, 0);
			}
	        }
	    }
	}
    }
}
//END MAIN ----------------------------------------------------------------------------------


// prompt() displays the command prompt and flushes stdout
void prompt()
{
    printf(": ");
    fflush(stdout);
}


//getUserInput() -- gets user input from stdin using getline
// RETURNS char* -- user input string
char* getUserInput()
{
    int numChars;
    char* inputBuffer = NULL;
    
    size_t bufferSize = 0;

    //prompts user for input until valid input received
    while (1)
    {
        prompt();
	
        numChars = -1;
        
        numChars = getline(&inputBuffer, &bufferSize, stdin);

	//if user just presses enter (only \n)
        if (numChars < 2)
        {
	    //clear
            clearerr(stdin);
        }

        //valid input breaks loop
        else
	{
	    break;
	}
    }	
    //returns input as char*
    return inputBuffer; 
}


//exitShell(DynArr*) -- exits smallsh, killing BG processes
// PARAMETERS: 
//           1) Pointer to DynaRR -- holding remaining BG process pids
void exitShell(DynArr* pArray)
{

    //loops through the pid array kill off remaining processes
    for (int i = 0; i < sizeDynArr(pArray); i++)
    {
        kill(getDynArr(pArray, i), SIGKILL);
    }

    //exits smallsh
    exit(0);
}


// changeDir(char*) -- changes the current working directory
// PARAMETERS:
//            1) char* -- path to new directory (absolute or relative)
void changeDir(char* path)
{
    //changes directory
    int result = chdir(path);
    char dirBuffer[255];

    //ERROR if chdir fails
    if (result == -1)
    {
        perror("Error");
    }
}


//getStatus() -- returns exit code or signel number of terminated child process
// PARAMETERS:
//         1) int childExitMethod of terminated process
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
// PARAMETERS:
//          1) char** of arguments to pass to execvp
void execute(char** argv)
{
    if (execvp(*argv, argv) < 0)
    {
        perror("Command Exec failure!");
	exit(1);
    }
}


// replacePid(char*) -- replaces instances of $$ with pid in passed in string
// PARAMETERS:
//           1) string
//
// RETURNS: new string with replacement
char* replacePid(char* text)
{
    //gets current pid and converts to int
    pid_t currentPid = getpid();
    int pid = (int) currentPid;

    //static buffer to hold new string
    static char buffer[4096];
    memset(buffer, '\0', sizeof(buffer));

    //stores location of $$ substring 
    char *location;

    //if $$ not found, returns original string
    if (!(location = strstr(text, "$$")))
        return text;

    //copies substring before $$ to buffer
    strncpy(buffer, text, location - text);

    //adds \0 to end of first substring
    buffer[location - text] = '\0';

    //prints pid and remaining string to end of buffer
    sprintf(buffer + (location - text), "%d%s", pid, location + 2);

    //returns the static buffer
    return buffer;
}


// reapBackground(DynArr*) --checks for terminated background processes, alerts user that they have been terminated
// PARAMETERS:
//         1) DynArr* to BGpid array
void reapBackground(DynArr* pArray)
{
    //stores pid of terminated child	
    pid_t cPid;
    int exitMethod;
    
    //loops through the pid array
    for (int i = 0; i < sizeDynArr(pArray); i++)
    {
	//reaps current process in the array    
        if ((cPid = waitpid(getDynArr(pArray, i), &exitMethod, WNOHANG)) > 0) 
        {
	    //removes pid of reaped process from the array	
            removeAtDynArr(pArray, i);

	    //prints message about reaped process
            printf("background pid %d is done: ", cPid);

	    //prints exit method of reaped process
	    getStatus(exitMethod);
	}	    
    }
}


// catchSTP -- called by signal handler for SIGTSTP.  Toggles foreground mode using global variable
void catchSTP(int signal)
{
    //display messages
    char* startMessage = "\nEntering foreground only mode (& is now ignored)\n";
    char* endMessage = "\nExiting foreground-only mode\n";

    //if FG only mode not activated
    if (stopped == 1)
    {
	//activates FG only mode, printing message    
        write(1, startMessage, 50);
	stopped = 0;
    }
    //if FG only mode activated
    else 
    {
	//deactivates FG only mode, printing message    
        write(1, endMessage, 30);
	stopped = 1;
    }
}
