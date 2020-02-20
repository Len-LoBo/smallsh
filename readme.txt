Program: smallsh 

Instructions For Command Line Compilation
----------------------------------------------------

Option 1) Using the included Makefile  (RECOMMENDED)
    
    From the program directory:
    -- type make

    To remove .o files and executable file
    -- type make clean


Option 2) Compiling using gcc at command line
    
   -- type gcc -o smallsh smallsh.c dynArr.c -std=gnu99
