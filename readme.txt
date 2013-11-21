******************************** README DISCDIAG *******************************

Discdiag is a disc oriented diagnostic. It runs in Windows
and linux. It needs administrator/root permission mode.

The usage instructions, and compilation instructions are in
the comments for the source, as well as the help function.

The files here are as follows:

cdiscdiag.bat      - compiles Windows and "pseudo linux" mode executables
                     under dos/windows.
cdiscdiag          - compiles the linux executable under linux.
discdiag           - The linux executable
discdiag.c         - The main source code for the diagnostic.
discdiag.exe       - Windows executable.
discdiag_linux.exe - Windows "linux pseudo executable".
discio.h           - General header file for OS specific functions.
linuxio.c          - The Linux I/O and specifics library.
readme.txt         - This file.
winio.c            - The Windows I/O specifics library.

In general, the file cdiscdiag contains the compile batch file. It is 
automatically correct for your OS, Windows or Linux, because Windows
will use the .bat version and linux the other.

The structure of the discdiag is simple, the main code is in discdiag.c
and all OS specific functions in winio.c or linuxio.c.

You have to have mingw installed to compile the diagnostic under windows.

Because mingw does a pretty good job of Unix emulation, it is close to
possible to run the Linux version under Windows. That is what the discdiag_linux
version is for. It is not a %100 working copy, but it serves as a test before 
moving the code to Linux for the final compile and test.
