Program: smallsh 
Description: A shell written in C with a limited number of built in commands.  Non-built in commands are passed to the OS.

Instructions For Command Line Compilation
----------------------------------------------------

Option 1) Using the included Makefile  (RECOMMENDED)
    
    From the program directory:
    > make

    To remove .o files and executable file
    > make clean


Option 2) Compiling using gcc at command line
    
   > gcc -o smallsh smallsh.c dynArr.c -std=gnu99
