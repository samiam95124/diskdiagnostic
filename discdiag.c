/** ****************************************************************************
*
* \file
*
* \brief This file contains the main diagnostic code
*
* \mainpage Disc Drive Diagnostic
*
* Operates on a Windows physical drive. Directly accesses sectors on the drive,
* placing the sector contents in a buffer. Accesses the drive at the lowest
* level, a Windows physical drive.
*
* The diagnostic maintains two buffers, one for reads and one for writes, which
* hold a large number of sectors (currently 256, but configurable). The idea is
* that you can set up patterns in the write buffer to be written out to disc,
* then read sectors into the read buffer for check, comparision or examination.
*
* The diagnostic is CLI oriented, and is "minimally scriptable". This means
* it supports multiple commands on a line, loops, variables, and other 
* abilities. The emphasis was on simple commands, oriented entirely to disc
* operations, and allowing as much to occur on a single command line as
* possible.
*
* Commands available:
* 
* ?, help                     - Print command help.
*
* r, read [lba][num]          - Read sector(s) at LBA, default read 0 1.  
*
* w, write [lba][num]         - Read sector(s) at LBA, default read 0 1.
*
* dw, dumpwrite [num]         - Dump sector(s) from write buffer, default 1.   
*
* dr, dumpread [num]          - Dump sector(s) from read buffer, default 1.   
*
* pt, pattn [pat [val [cnt]]] - Set write buffer to pattern, default is count.  
*
* c, comp [pat [val [cnt]]]   - Compare read buffer to pattern, default is count. 
* 
* cm, compmode mode           - Set miscompare handling mode.
*
* drive [num]                 - Set current physical drive, default is print current. 
*
* listdrives, ld              - List available physical drives. 
*
* unprot                      - Unprotect current drive. 
*
* echo [text]                 - Echo the parameter area with next line. 
* 
* echon [text]                - Echo the parameter area without next line.
*
* p, print ["fmt"] val...     - Print a calulated value with next line.
*
* pn, printn ["fmt"] val...   - Print a calulated value with NO next line.
*
* l, loop [num]               - Loop from line start num times, default is forever.
*
* lq, loopq [num]             - Loop quiet from line start num times, def. is for.
*
* u [num]                     - Loop until condition is true.
*
* while cond                  - Start while/wend loop. Exec loop if cond is true.
*
* wend                        - Terminate while/wend loop.
*
* repeat                      - Start repeat/until loop.
*
* until cond                  - End repeat/until loop. Repeat if cond is false.
*
* for var start end [step]    - Run for loop, start to end in var.  
*           
* fend                        - End for loop.
*
* select val                  - Select value, match successive cases for val.
*
* case val...                 - Start new select case.
*
* default                     - Start select case matching any value.
*
* send                        - Terminate select statement.
*
* if cond                     - Continue if condition met, otherwise next line.
*
* go label                    - Go to program label.
*
* end                         - Terminate procedure.
*
* s, set var val              - Set/reset user variable.
*
* local var                   - Mark variable as local.
*
* srand                       - Reset random number sequence.
*
* list                        - List stored program.
*
* clear                       - Clear stored program.
*
* save filename               - Save stored program to file.
*
* load filename               - Load stored program from file.
*
* delt num                    - Delete line in program with line number.
*
* exit                        - Exit diagnostic.
*
* exitonerror                 - Exit the diagnostic on error.
*
* i, input var                - Input value from user.
* 
* [option] Means an optional parameter.
*
* Multiple commands can appear on a line as a; b; c...
* 
* Patterns are:
* 
* cnt   - Byte incrementing count.
*
* dwcnt - 32 bit incrementing count.
*
* val   - Numeric 32 bit value, big endian.
*
* rand  - Random byte value.
*
* lba   - Only the first 32 bits get LBA, rest is $ff. LBA starts 
*         at [val], and increments across buffer. Note that this only
*         writes the first dword of each sector, use another pattern
*         to fill the background.
*
* buffs - Compare the read and write buffers to each other. This allows
*         complex patterns to be built up in the write buffer.
* 
* All write operations are from the write buffer.
*
* All read operations are from the read buffer.
* 
* All drives start write locked, and are relocked when the drive is changed.
* 
* User variables start with a-z and continue with a-z and 0-9 like Myvar1.
* They are created or recreated by set, and can be set any number of times.
* A variable can be used anywhere a val can.
* 
* There are several predefined variables:
* 
* drvsiz - Gives the size of the current physical drive.
*
* rand   - Gives a random number.
*
* lbarnd - Gives a random LBA for the current drive, ie., a random number
*          that fits into 0..drvsiz-1.
*
* secsiz - Size of sector in bytes (always 512).
*
* bufsiz - Size of read and write buffers in sectors.
*
* The compare modes are:
* 
* all - Show all mismatches.
*
* one - Show only the first mismatch.
*
* fail - Fail (abort) after the first mismatch (normal is continue).
*
* All numeric parameters can be expressions, using C style expression operators
* +a,-a,(a),a*b,a/b,a%b,a+b,a-b,a<b,a>b,a=b, a!=b,a<=b and a>b.
* Note that expressions cannot contain spaces, ie., a numeric parameter cannot
* have spaces within it.
*
* Format strings used with print must start and end with double quotes.
* They can contain standard C style format specifiers like %[w[.p]f, where
* the format character is d, x, or o, for decimal, hexadecimal or octal.
* The sign and length cannot be specified, since debug values are always signed
* long long values. Anything else in the format string is printed. There are
* no character escapes, use pn/printn and p/print to specify or leave out a 
* newline after printout.
*
* Note that leading zeros don't work in the field width. Use the precision 
* instead, i.e., %4.4x not %04x.
*
* Stored program lines are entered with a leading number as:
* 
* Diag> 1 dothis(num): echon The number is: ; p num
* 
* The line is inserted BEFORE the line in the program.
* 
* Any line with a label (as \"dothis:\" above) can be called as a
* procedure by using it's label as a command as:
* 
* Diag> dothis 42
* 
* Note parameters are optional.
* 
* Procedure execution stops with an \"end\" command, and execution
* resumes after the calling command.
* 
* All variables created in a procedure are temporary and removed at
* the end of the procedure. Variables are only created in a procedure
* if they don't exist outside of the procedure OR if \"local\" is used.
* Note that parameters are automatically local.
* 
* Note that hitting the end of the program buffer terminates the run
* ALWAYS.
* 
* The file \"discdiag.ini\", if present in the current directory, is
* automatically loaded when discdiag starts. If the file contains a
* procedure "init", that will be executed automatically on startup.
* 
* *** WARNING: This diagnostic CAN and WILL distroy your hard disc!
*
* Typical operations:
*
* diag> s lba 0
*
* diag> p lba; pt lba lba; w lba bufsiz; s lba lba+buzsiz; l 4096
*
* Writes the full drive with the "lba identify pattern", or the lba number as
* the first 32 bits (big endian) of each sector. The lba is printed each time
* through the loop. Note that "pt lba lba" means "pattern the write buffer with
* the lba pattern, and use lba numbers starting with the value in the variable
* lba". The first parameter of pt is the pattern name, the second is numeric/
* expression. 4096 is 1 megasector divided by 256, the size of the buffer.
*
* diag> s lba 0
*
* diag> p lba; r lba bufsiz; c lba lba; s lba lba+bufsiz; l 4096
*
* This verifies the lba pattern we just wrote.
*
* Bugs/wants/issues:
*
* 1. The calls used, readfile and writefile, hide errors from the drive. I know
* it is possible to retrieve actual errors from the drive, iometer does it. It
* would be far better if discdiag registered these errors (this may be a windows
* only issue).
*
* Compilation:
*
* Windows:
*
* I struggled with trying to get a standalone image from Visual studio. It does
* not work unless you carry the entire Visual studio environment to the target,
* which is not real practical. Cygwin has similar problems. The best way I 
* found is to get mingw and compile it:
*
* 1. Go to www.mingw.org and get the current mingw distribution. This will go
*    to c:\mingw by default.
*
* 2. Execute "c:\mingw\gcc -o discdiag discdiag.c". This is also in the file
*    cdiscdiag.bat.
*
* The result is .dll free and will run anywhere (yea).
*
* Linux:
*
* Compiles with the stock gcc compiler.
* 
******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <signal.h>
#include <time.h>
#include "discio.h"

#include <time.h>

/**
 *
 * Number of lines on screen (used to pause output)
 *
 */
#define LINES 24

/**
 *
 * Break flag
 *
 * Indicates ctl-c was hit on the console.
 *
 */
static int breakflag;

/**
 *
 * Sector write data buffer
 *
 * Writes take their data from here.
 *
 */
unsigned char writebuffer[SECSIZE*NOSECS];

/**
 *
 * Sector read data buffer
 *
 * Reads and writes put their data here.
 *
 */
unsigned char readbuffer[SECSIZE*NOSECS];

/**
 *
 * Current drive
 *
 * Stores a copy of the current drive number. This is -1 before any drive is
 * set.
 *
 */
int currentdrive;

/**
 *
 * Write protect
 *
 * This flag on write protects the drive. We set the write protect by default so
 * that it is not that easy to wipe the active drive.
 *
 */
int writeprot;

/**
 *
 * Current drive size
 *
 * This variable is loaded with the drive size when it is set active. It avoids
 * the need to perform a device call to get it each time.
 *
 * The drive size is in sectors.
 *
 */

long long drivesize;

/**
 *
 * Total IOPS write
 *
 * Tallies the total number of I/O operations for write.
 *
 */

double iopwrite;

/**
 *
 * Total IOPS read
 *
 * Tallies the total number of I/O operations for read.
 *
 */

double iopread;

/**
 *
 * Total bytes transferred on write
 *
 * Tallies the total bytes written to the current drive.
 *
 */

double bcwrite;

/**
 *
 * Total bytes transferred on read
 *
 * Tallies the total bytes written to the current drive.
 *
 */

double bcread;

/**
 *
 * Screen line counter
 *
 * Holds the next command line to parse.
 *
 */
int linecounter;

/**
 *
 * Command line buffer
 *
 * Holds the next command line to parse.
 *
 */
char linebuffer[250]; // Holder for command line

/**
 *
 * Buffer compare mode
 *
 */
typedef enum {

    /** Compare all */ compmode_all,
    /** Compare one */ compmode_one,
    /** Compare one and fail */ compmode_fail

} compmode;

/** Compare mode */                                        compmode curmode;
/** First miscompare flag */                               int first;
/** Repeat compare value */                                unsigned char comp_a; 
/** Repeat compare value */                                unsigned char comp_b; 
/** Repeat count */                                         int repcnt; 
/** Data that was set to compare values (comp_a, comp_b) */ int dataset; 
/** Exit diagnostic on error */                             int exiterror; 

/**
 *
 * Command result codes
 *
 * Defines result codes from command handlers. Not all non-zero codes mean 
 * errors, but all non-zero codes indicate special handling after the
 * command is complete.
 *
 */

typedef enum {

    /** Commmand terminated properly  */ result_ok, 
    /** "exit" command was seen       */ result_exit,
    /** Command terminated with error 
        (terminate batches and loops) */ result_error,
    /** Break (out of loop)           */ result_break,
    /** Continue (top of loop)        */ result_continue,
    /** Stop execution                */ result_stop,
    /** Restart line                  */ result_restart

} result;

/**
 *
 * Command definition structure
 *
 * Gives the name and what function to execute for each command.
 *
 */
typedef struct _command {

    /** Command verb string */ char *cmdstr;
    /** Command to execute */  result (*cmd)(char **line);

} command;

/*
 *
 * Command routine declarations
 *
 */
result command_help(char **line);
result command_read(char **line);
result command_write(char **line);
result command_dumpwrite(char **line);
result command_dumpread(char **line);
result command_pattn(char **line);
result command_comp(char **line);
result command_compmode(char **line);
result command_drive(char **line);
result command_listdrives(char **line);
result command_unprot(char **line);
result command_echo(char **line);
result command_echon(char **line);
result command_loop(char **line);
result command_loopq(char **line);
result command_untill(char **line);
result command_while(char **line);
result command_wend(char **line);
result command_repeat(char **line);
result command_until(char **line);
result command_if(char **line);
result command_go(char **line);
result command_for(char **line);
result command_fend(char **line);
result command_select(char **line);
result command_case(char **line);
result command_default(char **line);
result command_send(char **line);
result command_print(char **line);
result command_printn(char **line);
result command_set(char **line);
result command_local(char **line);
result command_srand(char **line);
result command_list(char **line);
result command_clear(char **line);
result command_save(char **line);
result command_load(char **line);
result command_delt(char **line);
result command_end(char **line);
result command_testrand(char **line);
result command_listvariables(char **line);
result command_exit(char **line);
result command_exitonerror(char **line);
result command_input(char **line);

/**
 *
 * Command table
 *
 * Gives each name and associated function for commands. Terminated by a dummy
 * entry with both null string and function.
 *
 */
command cmdtbl[] = {

    /** Help                 */      { "?",             command_help },
                                     { "help",          command_help },
    /** Read sector          */      { "r",             command_read },
                                     { "read",          command_read },
    /** Write sector         */      { "w",             command_write },
                                     { "write",         command_write },
    /** Dump write sector    */      { "dw",            command_dumpwrite },
                                     { "dumpwrite",     command_dumpwrite },
    /** Dump read sector     */      { "dr",            command_dumpread },
                                     { "dumpread",      command_dumpread },
    /** Set pattern          */      { "pt",            command_pattn },
                                     { "pattn",         command_pattn },
    /** Compare pattern      */      { "c",             command_comp },
                                     { "comp",          command_comp },
    /** Set compare mismatch mode */ { "cm",            command_compmode },
                                     { "compmode",      command_compmode },
    /** Set phy drive        */      { "drive",         command_drive },
    /** List physical drives */      { "listdrives",    command_listdrives },
                                     { "ld",            command_listdrives },
    /** Remove write protect */      { "unprot",        command_unprot },
    /** Echo text            */      { "echo",          command_echo },
    /** Echo text no newline */      { "echon",         command_echon },
    /** Loop                 */      { "l",             command_loop },
                                     { "loop",          command_loop },
    /** Loop quietly         */      { "lq",            command_loopq },
                                     { "loopq",         command_loopq },
    /** Until                */      { "u",             command_untill },
    /** While                */      { "while",         command_while },
    /** While end            */      { "wend",          command_wend },
    /** repeat               */      { "repeat",        command_repeat },
    /** until                */      { "until",         command_until },
    /** for                  */      { "for",           command_for },
    /** fend                 */      { "fend",          command_fend },
    /** select               */      { "select",        command_select },
    /** case                 */      { "case",          command_case },
    /** default              */      { "default",       command_default },
    /** send                 */      { "send",          command_send },
    /** Print                */      { "p",             command_print },
                                     { "print",         command_print },
    /** Print no newline     */      { "pn",            command_printn },
                                     { "printn",        command_printn },
    /** Set                  */      { "s",             command_set },
                                     { "set",           command_set },
    /** Local                */      { "local",         command_local },
    /** Reset random numbers */      { "srand",         command_srand },
    /** List program         */      { "list",          command_list },
    /** Clear program        */      { "clear",         command_clear },
    /** Save program         */      { "save",          command_save },
    /** Load program         */      { "load",          command_load },
    /** delete program line  */      { "delt",          command_delt },
    /** end routine          */      { "end",           command_end },
    /** go label             */      { "go",            command_go },
    /** if conditional       */      { "if",            command_if },
    /** Exit diagnostic      */      { "exit",          command_exit },
    /** Exit diagnostic on error */  { "exitonerror",   command_exitonerror },
    /** Input value */               { "i",             command_input },
                                     { "input",         command_input },
   
    /* Hidden test commands for the diagnostic */

    /** test random numbers  */      { "testrand",      command_testrand },
    /** list variables       */      { "listvariables", command_listvariables },

    /** End marker for command table */ { "", NULL }

};

/*
 * Variable handler definitions
 */
result variable_drvsiz(char **line, long long *ul);
result variable_rand(char **line, long long *ul);
result variable_lbarnd(char **line, long long *ul);
result variable_secsiz(char **line, long long *ul);
result variable_bufsiz(char **line, long long *ul);

/**
 *
 * Structure of a variable handler
 *
 * Gives the name and function hander for each variable.
 *
 */
typedef struct _variable {

    /** Variable name string */ char *varstr;
    /** Variable to execute */  result (*var)(char **line, long long *ll);

} variable;

/**
 *
 * Variable table
 *
 * Variables are just like commands, they get executed by name and can parse
 * from the command line. However, if they parse parameters they have to use
 * (x) form to keep the command line from being ambiguous.
 *
 * Variables also take a pointer to an unsigned long as the variable result.
 *
 */
variable vartbl[] = {

    /** Drive size                            */ { "drvsiz", variable_drvsiz },
    /** Random number                         */ { "rand",   variable_rand },
    /** Random number limited to LBA form     */ { "lbarnd", variable_lbarnd },
    /** Sector size in bytes                  */ { "secsiz", variable_secsiz },
    /** read and write buffer size in sectors */ { "bufsiz", variable_bufsiz },

    /** End marker for variable table */ { "", NULL }

};

/**
 *
 * User defined variable structure
 *
 * User defined variables are just a name and a value. They can be both read and
 * written.
 *
 */

typedef struct _uservar {

    /** Link to next in list */ struct _uservar *next;
    /** Variable name string */ char *varstr;
    /** Variable value */       long long val;

} uservar;

/** Root of user variables list */
uservar *varroot;

/**
 *
 * Loop count variable entry
 *
 */
typedef struct _loopcounter {

    /** Link to next in list */               struct _loopcounter* next;
    /** character position (for reference) */ char* pos;
    /** Loop count value */                   int loopcount;

} loopcounter;
    
/**
 *
 * Structure to hold stored text lines
 *
 */
typedef struct _linestr {

    /** Link to next in list */    struct _linestr *next;
    /** Label on line (if any) */  char *label;
    /** parameter list (if any) */ uservar *params;
    /** Text line */               char *line;
    /** loop counter */            loopcounter *looplist;

} linestr;

/** Root of stored execution text */

linestr *editroot;

/**
 *
 * Interpreter stack entries
 *
 */
typedef struct _intstk {

    /** next entry */                       struct _intstk *next;
    /** current line entry */               linestr *curlin;
    /** current character position there */ char *curchr;
    /** locals marker */                    uservar *mark;

} intstk;

/** Root of interpreter stack */

intstk *introot;

/**
 *
 * Control stack entry types.
 *
 */
typedef enum {

    /** while */  ctl_while,
    /** repeat */ ctl_repeat,
    /** for */    ctl_for

} ctltyp;

/**
 *
 * Control stack entries
 *
 * Controls are non-trivial controls that span multiple lines, like while/wend
 * and repeat/until.
 *
 */
typedef struct _ctlstk {

    /** next entry */         struct _ctlstk *next;
    /** line position */      linestr *linpos;
    /** character position */ char *chrpos;
    /** type */               ctltyp ctl;
    /** loop variable */      uservar *var;
    /** step value */         long long step;

} ctlstk;

/** root of controls stack */

ctlstk *ctlroot;

/*******************************************************************************

Support routines

*******************************************************************************/

/**
 *
 * Capture ctrl-c
 *
 * Turns a control-c press into a flag, which can then be checked by various
 * routines.
 *
 */
void ctlchandler(int sig)

{

    signal(SIGINT, ctlchandler);
    signal(SIGABRT, ctlchandler);

    breakflag = 1; // set break occurred

}

/**
 *
 * Check user break
 *
 * Check if a user break occurred. Returns true if so.
 */
int chkbrk(void)

{

    int breakflags; // save for break flag

    breakflags = breakflag; // save contents of break flag

    breakflag = 0; // clear any break

    return breakflags; // return state of user break

}

/**
 *
 * Get high resolution timer
 *
 * Get current setting on high resolution timer.
 *
 */
long long gettim(void)

{

    long long time;

    time = clock();

    return time;

}

/**
 *
 * Find elapsed time in seconds
 *
 * Finds the elapsed time in seconds. Returns as a floating point value so that
 * fractional times can be represented.
 *
 */
double elapsed(
    /** reference time */ long long t
)

{

    double etim;

    etim = (gettim()-t*1.0)/CLOCKS_PER_SEC;

    return etim;

}

/**
 *
 * Get line from file
 *
 * Gets a text line from the given file into the buffer with length.
 * Any line ending is removed from the buffer.
 *
 * \returns true on eof, otherwise false
 *
 */

int readline(
    /** File to read from */ FILE *fp, 
    /** Buffer for string */ char *buff,
    /** Size of buffer    */ int size
)

{

    char *sr; // string return
    int s;

    // Terminate line in case there is a user break. This means that
    // it won't just exit with the contents of the previous buffer.
    *buff = 0;
    sr = fgets(buff, size, fp); // read a line
    if (sr) { // not eof

        s = strlen(buff); // find length
        if (s) { // string not empty

            // if the last character is newline, knock it out
            if (buff[s-1] == '\n') buff[s-1] = 0;
            s--; // adjust length
            // on linux, the end is cr-lf
            if (s) { // string not empty

                // if the last character is newline, knock it out
                if (buff[s-1] == '\r') buff[s-1] = 0;

            }

        }

    }

    return !sr; // return true if string NULL

}

/** Seed for random number generator */ unsigned long seed = 1; 

/**
 *
 * Return the next 32 bit random number
 * 
 * \returns 32 bit random number 
 *
 */ 
unsigned long rand32() { 
 
    unsigned long long tmpseed;
    unsigned long mlcg,p,q;
    
    tmpseed =  (unsigned long long)33614U * (unsigned long long)seed; 
    q = (unsigned long) tmpseed;  /* low */ 
    q = q >> 1; 
    p = tmpseed >> 32 ;           /* hi */ 
    mlcg = p + q; 
    if (mlcg & 0x80000000) {
    
        mlcg = mlcg & 0x7FFFFFFF; 
        mlcg++; 
  
    } 
    seed = mlcg; 
 
    return mlcg;
    
} 

/**
 *
 * Return the next 64 bit random number
 *
 * \returns 64 bit random number
 *
 */ 
long long rand64() {

    return (((long long) rand32() & 0x7fffffff) << 32) | rand32();

}

/**
 *
 * Process screen pause
 *
 * Increments the screen counter, then outputs a pause message if
 * it exceeds the number of lines on a screen.
 *
 */

void pause(void)

{

    linecounter++;
    if (linecounter > LINES-1) {

        printf("*** Hit return to continue ***");
        getchar();
        linecounter = 0;

    }

}

/**
 *
 * Dump buffer
 *
 * Dumps the given buffer in hex and ASCII
 *
 * \returns Standard discdiag error code.
 *
 */

result dump(
    /* Buffer to dump */ unsigned char *buffer,
    /* Size of buffer */ long long size
)

{

    int i, x, b;
    result r;

    r = result_ok; // set result code ok
    linecounter = 0; // reset screen line counter
    for (b = x = i = 0; i < size; i++) {

        if (!(i % 16)) printf("%8.8x: ", i);
        printf("%2.2x ", buffer[i]);
        b++; // count bytes
        if (i % 16 == 15) { // end of line
            
            printf(" \"");
            while (x < i) {
                
                if ((buffer[x] & 0x7f) >= ' ') printf("%c", buffer[x] & 0x7f);
                else printf(".");
                x++;

            }
            printf("\"\n");
            pause();
            if (chkbrk()) {
            
                if (exiterror) r = result_exit; // exit diagnostic
                goto stop; // stop on break
                
            }
            b = 0; // clear byte count
            if (chkbrk()) return result_stop; // check break

        }
        
    }
    if (x < i-1) { // finish incomplete last line

        // fill in the missing line data
        for (i = 0; i < (16-b); i++) printf("   ");
        printf(" \"");
        while (x < i) {
                
            if ((buffer[x] & 0x7f) >= ' ') printf("%c", buffer[x] & 0x7f);
            else printf(".");
            x++;

        }
        printf("\"\n");

    }
    stop:
  
    return r; // exit with result code

}

/**
 *
 * Print number with multiplier
 *
 * Prints a number scaled by M (mega), k (kilo) or nothing.
 *
 */

void printscaled(
    /** Number to print */ double n
)

{

   if (n > 1024.0*1024.0) printf("%.2fM", n/(1024.0*1024.0));
   else if (n > 1024.0) printf("%.2fk", n/1024.0);
   else printf("%.2f", n);

}

/**
 *
 * Print number scaled and per second.
 *
 * Prints the given number of the form:
 *
 * label: n (q/s)
 *
 * Where N is the total and q is the rate per second.
 *
 */

void printscpersec(
    /** Label to print */ char labl[],
    /** Number */         double n,
    /** Time */           double time
)

{

    printf("%s", labl); 
    printscaled(n); 
    printf(" ("); 
    if (time == 0) printscaled(0); else printscaled(n/time); 
    printf("/s) ");

}

/**
 *
 * Get word off command line
 *
 * Retrieves the next space delimited word from the passed command line.
 * Can be used as a general purpose parameter parser.
 *
 */
void getword(
    /** Line to parse */      char **l,
    /** Word output buffer */ char *w
)

{

    while (**l == ' ') (*l)++; // skip any leading spaces
    // copy non-space to buffer
    while (**l && (isalnum(**l) || **l == '?' || **l == '.')) 
        *w++ = *(*l)++;
    *w = 0; // terminate buffer

} 

/**
 *
 * List variables stack
 *
 * Dumps the contents of the variables stack in LI order. A diagnostic.
 *
 */
void listvar(void)

{

    uservar *p;

    // search the user variables list
    p = varroot; // index root
    while (p) {

        printf("%s: var: %s val: %lld\n", __FUNCTION__, p->varstr, p->val);
        p = p->next; // link next entry

    }

}

/**
 *
 * Find user variable
 *
 * Searches for a user variable by name. Returns a pointer to the variable
 * entry or NULL if not found.
 *
 * \returns User variable entry.
 *
 */
uservar *fndvar(
    /** Name of user variable */ char *name
)

{

    uservar *puvar;
    uservar *pufnd;

    /* search the user variables list */
    puvar = varroot; // index root
    pufnd = NULL; // set not found
    while (puvar) {

        if (!strcmp(name, puvar->varstr)) { // found

            pufnd = puvar; // set found entry
            puvar = NULL; // flag found

        } else puvar = puvar->next; // link next entry

    }

    return pufnd; // return entry or NULL

}

/**
 *
 * Enter new user variable
 *
 * Creates and pushes a new variable on the variables stack with the given name
 * and value.
 *
 * \returns Standard discdiag error code.
 * 
 */
result pushvar(
    /** Name of variable */ char *name,
    /** Value for variable */ long long val
)

{

    uservar *uvar; // pointer to user variable entry
    int s;

    // get a new entry
    uvar = (uservar *) malloc(sizeof(uservar));
    s = strlen(name); // find length of name
    uvar->varstr = (char *) malloc(s+1);
    if (!uvar || !name) {

        printf("*** Error: Cannot allocate space\n");

        return result_error;

    }
    strncpy(uvar->varstr, name, s+1); // place name in string
    uvar->val = val; // place value
    uvar->next = varroot; // push onto root list
    varroot = uvar;

    return result_ok;

}

/**
 *
 * Get number off command line
 *
 * Gets a number off the command line. If no number is found, returns -1.
 *
 * \returns Standard discdiag error code.
 * 
 */
result getval(
    /** Line to parse */      char **l,
    /** Number returned */    long long *n
)

{

    char w[100]; // buffer for parameter
    variable *var; // pointer to variable table
    result r;
    int found;
    uservar *uvar;

    getword(l, w); // get next word
    if (isalpha(*w)) { // is variable

        var = vartbl; // index start of variable table
        found = 0; // set nothing found yet
        while (var && var->var) { // traverse until end marker seen
    
            // If the variable matches, execute it
            if (!strcmp(w, var->varstr)) {
    
                r = var->var(l, n); // execute variable
                // check valid
                if (r != result_ok) return r;
                found = 1; // set variable was found
                var = NULL; // flag found
    
            }
            if (var) var++; // next command entry
    
        }
        if (!found) {

            /* try searching the user variables list */
            uvar = fndvar(w);
            if (!uvar) { // nothing found
     
                printf("*** Error: Variable \"%s\" invalid\n", w);

                return result_error; // bad value

            }
            *n = uvar->val; // return user variable  
    
        }

    } else if (isdigit(*w)) {

        // Note Microsoft does not support strtoull
        *n = (long long)strtoul(w, NULL, 0); // return resulting number

    } else {
      
        printf("*** Error: Invalid value\n", w);

        return result_error; // bad value

    }

    return result_ok;

} 

result getparam(char **l, long long *n);

/**
 *
 * Process factor
 *
 * Processes factor. The factors are:
 *
 * +a
 *
 * -a
 *
 * (a)
 *
 * \returns Standard discdiag error code.
 * 
 */
result getfact(
    /** Line to parse */   char **l,
    /** Number returned */ long long *n
)

{

    long long v;
    result r;

    if (**l == '+') { // +a

        (*l)++; // skip 
        r = getfact(l, &v); // get subexpression
        if (r != result_ok) return r;
        *n = +v; // find +a

    } else if (**l == '-') { // -a

        (*l)++; // skip -
        r = getfact(l, &v); // get subexpression
        if (r != result_ok) return r;
        *n = -v; // find -a

    } else if (**l == '(') { // (a)

        (*l)++; // skip (
        r = getparam(l, n); // get subexpression
        if (r != result_ok) return r;
        while (**l == ' ') (*l)++; // skip spaces
        if (**l != ')') {

            printf("*** Error: ')' expected\n");
 
            return result_error;

        }
        (*l)++; // skip )

    } else {

        r = getval(l, n); // get value
        if (r != result_ok) return r;

    }

    return result_ok; // exit good 

}

/**
 *
 * Process multiply class operator
 *
 * Processes a multiply or divide. The expressions are:
 *
 * a * b
 *
 * a / b
 *
 * a % b
 *
 * \returns Standard discdiag error code.
 * 
 */
result getmult(
    /** Line to parse */   char **l,
    /** Number returned */ long long *n
)

{

    long long v;
    result r;

    /* get left side expression */
    r = getfact(l, n);
    if (r != result_ok) return r;
    while (**l == '*' || **l == '/' || **l == '%') {

        if (**l == '*') { // *
    
            (*l)++; // skip *
            r = getfact(l, &v); // get right
            if (r != result_ok) return r;
            *n = *n * v; // find a * b
    
        } else if (**l == '/') { // /
    
            (*l)++; // skip /
            r = getfact(l, &v); // get right
            if (r != result_ok) return r;
            if (v == 0) { // check zero divide

                printf("*** Error: Zero divide\n");

                return result_error;

            }
            *n = *n / v; // find a / b
    
        } else if (**l == '%') { // %
    
            (*l)++; // skip %
            r = getfact(l, &v); // get right
            if (r != result_ok) return r;
            if (v == 0) { // check zero divide

                printf("*** Error: Zero divide\n");

                return result_error;

            }
            *n = *n % v; // find a % b
    
        }

    }

    return result_ok; // exit good 

}

/**
 *
 * Process add class operator
 *
 * Processes an add or subtract. The expressions are:
 *
 * a + b
 *
 * a - b
 *
 * \returns Standard discdiag error code.
 * 
 */
result getadd(
    /** Line to parse */   char **l,
    /** Number returned */ long long *n
)

{

    long long v;
    result r;

    /* get left side expression */
    r = getmult(l, n);
    if (r != result_ok) return r;
    while (**l == '+' || **l == '/' || **l == '-') {

        if (**l == '+') { // +
       
            (*l)++; // skip +
            r = getmult(l, &v); // get right
            if (r != result_ok) return r;
            *n = *n + v; // find a + b
       
        } else if (**l == '-') { // -
       
            (*l)++; // skip -
            r = getmult(l, &v); // get right
            if (r != result_ok) return r;
            *n = *n - v; // find a - b
       
        }

    }

    return result_ok; // exit good 

}

/**
 *
 * Process parameter
 *
 * Processes parameter expressions. The expressions are:
 *
 * a > b
 *
 * a < b
 *
 * a = b
 *
 * a != b
 *
 * a >= b
 *
 * a <= b
 *
 * \returns Standard discdiag error code.
 * 
 */
result getparam(
    /** Line to parse */      char **l,
    /** Number returned */    long long *n
)

{

    long long v;
    result r;

    /* get left side expression */
    r = getadd(l, n);
    if (r != result_ok) return r;
    if (**l == '>') { // > or >=

        (*l)++; // skip >
        if (**l == '=') { // >=

            (*l)++; // skip =
            r = getadd(l, &v); // get right
            if (r != result_ok) return r;
            *n = *n >= v; // find a >= b

        } else {

            r = getadd(l, &v); // get right
            if (r != result_ok) return r;
            *n = *n > v; // find a > b

        }

    } else if (**l == '<') { // < or <=

        (*l)++; // skip <
        if (**l == '=') { // >=

            (*l)++; // skip =
            r = getadd(l, &v); // get right
            if (r != result_ok) return r;
            *n = *n <= v; // find a <= b

        } else {

            r = getadd(l, &v); // get right
            if (r != result_ok) return r;
            *n = *n < v; // find a < b

        }

    } else if (**l == '=') { // =

        (*l)++; // skip =
        r = getadd(l, &v); // get right
        if (r != result_ok) return r;
        *n = *n == v; // find a = b

    } else if (**l == '!') { // !=

        (*l)++; // skip !
        if (**l != '=') {

            // this could be a comment, we need to back out
            (*l)--;

        } else {

            (*l)++; // skip =
            r = getadd(l, &v); // get right
            if (r != result_ok) return r;
            *n = *n != v; // find a != b

        }

    }

    return result_ok; // exit good 

}

/**
 *
 * Print comparision
 *
 * Accepts two byte values, and prints out if they miscompare.
 *
 * \returns Standard discdiag error code.
 * 
 */
result printcomp(
    /** Buffer address */      long addr,
    /** New (read) byte */     unsigned char nb, 
    /** Old (existing) byte */ unsigned char ob
)

{

    result r;
    
    r = result_ok; // set result code ok
    if (nb != ob) {

        // check is first miscompare or mode is print all
        if (first || curmode == compmode_all) {

            if (dataset && nb == comp_a && ob == comp_b) {

                repcnt++; // just count

            } else {

                // message if miscompares have accumulated
                if (repcnt) {

                    printf("*** Info: There were %d occurrances of the above mismatch\n", repcnt);
                    repcnt = 0; // reset counter

                }
                printf("*** Error: Buffer miscompare: %8.8lx: %2.2x s/b %2.2x\n", 
                       addr, nb, ob);

            }

        }

        first = 0; // set no longer first compare

        // return fatal if mode is fail miscompare
        if (curmode == compmode_fail) return result_error;

        // place last compare values
        comp_a = nb;
        comp_b = ob;
        dataset = 1;

    }
    if (chkbrk()) {
    
        if (exiterror) r = result_exit; // exit diagnostic
        else r = result_stop; // check break
        
    }

    return r; // return result code

}

/**
 *
 * Enter text line
 *
 * Enters a line to the edit list. If the line has a number in front, that is
 * used to sequence the line. The line number will be stripped off and the line
 * will be placed at that position in the edit buffer. Otherwise, it will be
 * placed at the end of the edit list. Typically, the user will place the number
 * there, which both signals the line is to be stored, as well as where. In a
 * file, the lines are simply dumped into the edit store at the end. If there
 * are numbers in the loaded file, they will cause the lines to be ordered
 * according to number, which is unlikely what the user wanted, but is the
 * correct thing. In any case, a line with a leading number is not a valid
 * command.
 *
 * \returns Standard discdiag error code.
 * 
 */
result enterline(char *line)

{

    long long n;
    linestr *p, *l, *p2;
    int s;
    result r;
    char *lines;
    char *label;
    char cbuf[100];
    uservar *params, *params2, *params3, *vp;
    char w[100];

    n = -1; // set impossible line
    lines = line; // save line position
    while (*line == ' ') line++; // skip spaces
    // check leading number exists
    if (isdigit(*line)) {

        r = getval(&line, &n); // get the number
        if (r != result_ok) return r; // error

    } else line = lines; // no line number, back up to start
    // look ahead for label
    label = NULL; // set no label
    params = NULL;
    lines = line; // save line position
    while (*line == ' ') line++; // skip spaces
    if (isalpha(*line)) { // if there is an alpha label

        getword(&line, cbuf); // get the label
        while (*line == ' ') line++; // skip spaces
        if (*line == ':' || *line == '(') { // its a label

            // if the form is: "label:" at the start of line, its a label. We
            // strip this off and set it to a special field in the text line
            // tracking entry.
            if (*line == '(') { // parameter list

                line++; // skip '('
                while (*line == ' ') line++; // skip spaces
                while (*line && *line != ')' && *line != ':') {

                    getword(&line, w); // get a parameter
                    s = strlen(w); // get length
                    if (!s) { // flag bad parameter

                        printf("*** Error: Bad parameter specification\n");
                        return result_error;

                    }
                    // get a new variable entry
                    vp = (uservar *) malloc(sizeof(uservar));
                    // push to parameters list
                    vp->next = params;
                    params = vp;
                    // allocate parameter label
                    vp->varstr = (char *) malloc(s+1);
                    strncpy(vp->varstr, w, s+1); // place label
                    vp->val = 0; // set initializer value

                }
                // reverse the parameter list into order
                params2 = NULL;
                while (params != NULL) { // drain the list

                    params3 = params; // remove from source list
                    params = params->next;
                    params3->next = params2; // push to destination
                    params2 = params3;
                   
                }
                params = params2; // place final list
                if (*line != ')') { // bad ending

                    printf("*** Error: ')' expected\n");
                    return result_error;

                } else line++; // skip ')'
                while (*line == ' ') line++; // skip spaces
                if (*line != ':') { // no ':'

                    printf("*** Error: ':' expected\n");
                    return result_error;

                }
                
            }
            line++; // skip ':'
            s = strlen(cbuf); // find length of label
            label = (char *) malloc(s+1); // allocate plus zero
            strncpy(label, cbuf, s+1); // place in label

        } else {

            // we failed to find a label, back out of the deal
            line = lines;

        }

    } else {

        // we failed to find a label, back out of the deal
        line = lines;

    }

    p = editroot;
    l = NULL;
    while (--n && p) { l = p; p = p-> next; } // find line by numeric sequence 
    // insert line to present position
    p2 = (linestr *) malloc(sizeof(linestr)); // get new entry
    if (!l) { p2->next = editroot; editroot = p2; } // list was empty or first
    else { p2->next = l->next; l->next = p2; } // append/insert here
    // Now fill in the entry
    p2->label = label; // set label
    p2->params = params; // set parameters
    s = strlen(line); // find remaining length of line
    p2->line = (char *) malloc(s+1); // allocate with trailing zero
    strncpy(p2->line, line, s+1); // place text line
    p2->looplist = NULL; // clear loop counter list

    return result_ok; 

}

/**
 *
 * Clear program
 *
 * Clear current program base and recycle all entries.
 *
 */

void clrpgm(void)

{

    linestr *p;

    while (editroot) { // empty list
 
        p = editroot; // index top line
        editroot = p->next; // gap out
        if (p->label) free(p->label); // free label if exists
        free(p->line); // free text line
        free(p); // free the text header

    }

}

/**
 *
 * Load program from file
 *
 * Loads a program from a given file. Returns 0 for success.
 *
 * \returns 0 if the file loaded, otherwise 1.
 *
 */

int loadfile(
    /** Filename to load */ char *fname
)

{

    FILE *fp;
    char buffer[250];
    int r;

    // start by clearing out and recycling the old progam
    fp = fopen(fname, "r");
    if (!fp) return 1; // couldn't open file    
    clrpgm(); // clear existing program out
    do { // read lines

       r = readline(fp, buffer, sizeof(buffer)); // read a line 
       // if not eof, enter the line
       if (!r) enterline(buffer);

    } while (!r); // until eof
    fclose(fp);

    return 0; // return result ok

}

/**
 *
 * Push new interpreter level
 *
 * Adds a new level to the interpreter stack using the line buffer and character
 * position given.
 *
 */
void pushlvl(
    /** Current line buffer */ linestr *line,
    /** Current character position within buffer */ char *cpos
)

{

    intstk *p;

    p = (intstk *) malloc(sizeof(intstk)); // get a new stack entry
    p->next = introot; // push onto stack
    introot = p;
    p->curlin = line; // set buffer
    p->curchr = cpos; // set character position
    p->mark = varroot; // mark locals
   
}

/**
 *
 * Pop old interpreter level
 *
 * Removes an interpreter level. Note that the interpreter stack should never
 * go dry.
 *
 */
void poplvl(void)

{

    intstk *p;
    uservar *vp;

    if (!introot) {

        printf("*** Error: System fault: Interpreter stack runs dry\n");
        printf("***        Halting program\n");

        exit(1);

    }
    // remove locals if present, and we are not in immediate mode
    if (introot && introot->next && introot->mark) {

        while (varroot && introot->mark != varroot) {

            vp = varroot; // index top variable entry
            varroot = vp->next; // gap out
            if (vp->varstr) free(vp->varstr); // release label string
            free(vp); // release entry

        } 

    }
    // remove stack entry
    p = introot; // index top entry
    introot = p->next; // gap out
    free(p); // recycle entry

}

/**
 *
 * Find program label
 *
 * Searches for a label in the program store using the given name, and returns
 * that if found, otherwise NULL.
 *
 * \returns The label string.
 *
 */
linestr *fndpgm(
    /** Name of user variable */ char *name
)

{

    linestr *p, *pf;

    /* search the user variables list */
    p = editroot; // index root
    pf = NULL; // set not found
    while (p) {

        if (p->label && !strcmp(name, p->label)) { // found

            pf = p; // set found entry
            p = NULL; // flag found

        } else p = p->next; // link next entry

    }

    return pf; // return entry or NULL

}

/**
 *
 * Find line counter
 *
 * Given a line count list, either finds an existing line counter, or adds a new
 * one. The line count entry is identified by the line character position, which
 * is passed. That is, each loop command will have a unique place on the command
 * line, and the counter for it is filed using that position.
 *
 * \returns The loopcounter entry.
 *
 */
loopcounter* fndcnt(
    /** Root of counter list */       loopcounter** list,
    /** Character position of loop */ char* pos)

{

    /* loopcounter list pointer */  loopcounter *p; 
    /* loopcounter found pointer */ loopcounter *f; 

    f = NULL; // set no entry found
    p = *list; // index top of list
    while (p != NULL) {

        if (p->pos == pos) { // found

            f = p; // set found entry
            p = NULL; // stop search

        } else // not found
            p = p->next; // go next entry in list

    }
    if (f == NULL) { // no entry found

        // create new line counter entry
        f = (loopcounter*) malloc(sizeof(loopcounter));
        f->next = *list; // push to top of target list
        *list = f;
        f->pos = pos; // set character position to match
        f->loopcount = 0; // clear counter

    }

    return f; // return found/made entry

}

/**
 *
 * Reset line counters
 *
 * Resets all of the program line counters to zero
 *
 */

void rstlin(void)

{

    linestr *p;
    loopcounter* cp;

    p = editroot; // index first line
    while (p) { // traverse and print

        cp = p->looplist; // index top of loopcounter list
        while (cp != NULL) { // cross the list 

            cp->loopcount = 0; // reset counter
            cp = cp->next; // next entry

        }
        p = p->next; // go next line in list

    }

}

/**
 *
 * Pop control level
 *
 * Removes and frees one control level
 *
 */
void popctl(void)

{

    ctlstk *cp;

    if (ctlroot) { // there is a level

        cp = ctlroot; // index top entry 
        ctlroot = cp->next; // gap out
        free(cp); // free up

    }

}

/**
 *
 * Skip commands
 *
 * Skip commands until the recognized one is found. Takes a command verb,
 * and skips forward until that command verb is found. Expects to be at the
 * end of a command, which means that the next ';' command separator or end
 * of line is to be found, then the commands after that examined.
 *
 * \returns Standard discdiag error code.
 * 
 */
result skipcmd(
    /** Remaining command line */ char **line,
    /** Command to skip */        char *cmd,
    /** Command to skip 2 */      char *cmd2,
    /** Command to skip 3 */      char *cmd3,
    /** What label was found */   int  *fnd
)

{

    char w[100]; // word buffer
    int whlcnt; // nested while counter
    int repcnt; // nested repeat counter
    int forcnt; // nested for counter
    int selcnt; // nested select counter

    whlcnt = 0; // clear nesting counts
    repcnt = 0;
    forcnt = 0;
    selcnt = 0;
    // skip the rest of the command/parameters
    while (**line && **line != ';') (*line)++; // skip until ';' or end
    if (**line == ';') (*line)++; // skip ';'
    do { // skip lines in program

        // skip commands on line
        do {

            while (**line && **line == ' ') (*line)++; // skip spaces
            if (**line) { // line is not empty

                getword(line, w); // find command verb
                // update nesting counts
                if (!strcmp(w, "while"))  whlcnt++;
                if (!strcmp(w, "wend"))   { whlcnt--; if (whlcnt < 0) whlcnt = 0; }
                if (!strcmp(w, "repeat")) repcnt++;
                if (!strcmp(w, "until"))  { repcnt--; if (repcnt < 0) repcnt = 0; }
                if (!strcmp(w, "for"))    forcnt++;
                if (!strcmp(w, "fend"))   { forcnt--; if (forcnt < 0) forcnt = 0; }
                if (!strcmp(w, "select")) selcnt++;
                if (!strcmp(w, "send"))   { selcnt--; if (selcnt < 0) selcnt = 0; }
                // lock out nested sections. We treat excesss ends as no-ops,
                // but at that point the algorithim is suspect in any case.
                // It means the source is playing games with the nesting.
                if (!(whlcnt || repcnt || forcnt || selcnt)) {

                    // search for provided words
                    *fnd = 1; // set find 1
                    if (!strcmp(w, cmd)) return result_ok; // found
                    *fnd = 2; // set find 2
                    if (!strcmp(w, cmd2)) return result_ok; // found
                    *fnd = 3; // set find 3
                    if (!strcmp(w, cmd3)) return result_ok; // found

                }
                while (**line && **line != ';') (*line)++; // skip until ';' or end
                if (**line == ';') (*line)++; // skip ';'

            }

        } while (**line); // not end of line
        if (introot && introot->next) {

            // not in immediate mode, advance to next line in program
            introot->curlin = introot->curlin->next;
            // if not end of program, set start of new line
            if (introot->curlin) *line = introot->curlin->line;
            else {

                // end of program, flush stack and bail
                while (introot) poplvl();
                return result_error;

            }

        }

    } while (introot);

    return result_error;

}

/*******************************************************************************

Variable handlers

*******************************************************************************/

/**
 *
 * Drive size
 *
 * Return current size of drive
 *
 * \returns Standard discdiag error code.
 * 
 */

result variable_drvsiz(
    /** Remaining command line */ char **line,
    /** Returned value */         long long *ll
)

{

    long long t;
    int r;

    *ll = drivesize; // return drive size

    return result_ok; // return no fault

}

/**
 *
 * Random number
 *
 * Returns a random number.
 *
 * Note that this is currently a 32 bit implementation, we need to extend
 * to 64 bits. This is required to handle larger drives.
 *
 * \returns Standard discdiag error code.
 * 
 */

result variable_rand(
    /** Remaining command line */ char **line,
    /** Returned value */         long long *ll
)

{

    *ll = rand64();

    return result_ok; // return no fault

}

/**
 *
 * Random LBA
 *
 * Returns a random number limited to the LBA size
 *
 * \returns Standard discdiag error code.
 * 
 */

result variable_lbarnd(
    /** Remaining command line */ char **line,
    /** Returned value */         long long *ll
)

{

    char *dummystr = "";
    result r;
    long long drivesize;
    

    r = variable_drvsiz(&dummystr, &drivesize);
    if (r == result_ok)
        *ll = rand64() % drivesize;

    return r;

}

/**
 *
 * Sector size
 *
 * Returns the sector size. This is pretty much set at 512 since the dawn of
 * fire.
 *
 * \returns Standard discdiag error code.
 * 
 */

result variable_secsiz(
    /** Remaining command line */ char **line,
    /** Returned value */         long long *ll
)

{

    char *dummystr = "";
    
    *ll = SECSIZE;

    return result_ok;

}

/**
 *
 * Buffer size
 *
 * Returns the size, in sectors, of both the read and the write buffer.
 *
 * \returns Standard discdiag error code.
 * 
 */

result variable_bufsiz(
    /** Remaining command line */ char **line,
    /** Returned value */         long long *ll
)

{

    char *dummystr = "";
    
    *ll = NOSECS;

    return result_ok;

}

/*******************************************************************************

Command handlers

*******************************************************************************/

/**
 *
 * Help
 *
 * Print command menu.
 *
 * \returns Standard discdiag error code.
 * 
 */

result command_help(
    /** Remaining command line */ char **line
)

{

    linecounter = 0; // clear screen line count
    printf("\n"); pause();
    printf("Commands available:\n"); pause();
    printf("\n"); pause();
    printf("?, help                     - Print command help.\n"); pause();
    printf("r, read [lba][num]          - Read sector(s) at LBA, default read 0 1.\n"); pause();
    printf("w, write [lba][num]         - Read sector(s) at LBA, default read 0 1.\n"); pause();
    printf("dw, dumpwrite [num]         - Dump sector(s) from write buffer, default 1.\n"); pause();
    printf("dr, dumpread [num]          - Dump sector(s) from read buffer, default 1.\n"); pause();
    printf("pt, pattn [pat [val [cnt]]] - Set write buffer to pattern, default is count.\n"); pause();
    printf("c, comp [pat [val [cnt]]]   - Compare read buffer to pattern, default is count.\n"); pause();
    printf("cm, compmode mode           - Set miscompare handling mode, default is one.\n"); pause();
    printf("drive [num]                 - Set current phy drive, default is print current.\n"); pause();
    printf("listdrives, ld              - List available physical drives.\n"); pause();
    printf("unprot                      - Unprotect current drive.\n"); pause();
    printf("echo [text]                 - Echo the parameter area with next line.\n"); pause();
    printf("echon [text]                - Echo the parameter area without next line.\n"); pause();
    printf("p, print [fmt] val...       - Print calulated value(s) with next line.\n"); pause();
    printf("pn, printn [fmt] val...     - Print calulated value(s) without next line.\n"); pause();
    printf("l, loop [num]               - Loop from line start num times, default is\n"); pause();
    printf("                              forever.\n"); pause();
    printf("lq, loopq [num]             - Loop quiet from line start num times, defult is\n"); pause();
    printf("                              forever.\n"); pause();
    printf("u [num]                     - Loop until condition is true.\n"); pause();
    printf("while cond                  - Start while/wend loop. Exec loop if cond is true.\n"); pause();
    printf("wend                        - Terminate while/wend loop.\n"); pause();
    printf("repeat                      - Start repeat/until loop.\n"); pause();
    printf("until cond                  - End repeat/until loop. Repeat if cond is false.\n"); pause();
    printf("for var start end [step]    - Run for loop, start to end in var.\n");              pause();
    printf("fend                        - End for loop.\n"); pause();
    printf("select val                  - Select value, match successive cases for val.\n"); pause();
    printf("case val                    - Start new select case.\n"); pause();
    printf("default                     - Start select case matching any value.\n"); pause();
    printf("send                        - Terminate select statement.\n"); pause();
    printf("end                         - Terminate procedure.\n"); pause();
    printf("go label                    - Go to program label.\n"); pause();
    printf("if cond                     - Continue if condition met, otherwise next line.\n"); pause();
    printf("s, set var val              - Set/reset user variable.\n"); pause();
    printf("local var                   - Mark variable as local.\n"); pause();
    printf("srand                       - Reset random number sequence.\n"); pause();
    printf("list                        - List stored program.\n"); pause();
    printf("clear                       - Clear stored program.\n"); pause();
    printf("save filename               - Save stored program to file.\n"); pause();
    printf("load filename               - Load stored program from file.\n"); pause();
    printf("delt num                    - Delete line in program with line number.\n"); pause();
    printf("exit                        - Exit diagnostic.\n"); pause();
    printf("exitonerror                 - Exit the diagnostic on error.\n"); pause();
    printf("\n"); pause();
    printf("[option] Means an optional parameter.\n"); pause();
    printf("\n"); pause();
    printf("Multiple commands can appear on a line as a; b; c...\n"); pause();
    printf("\n"); pause();
    printf("Patterns are:\n"); pause();
    printf("\n"); pause();
    printf("cnt   - Byte incrementing count.\n"); pause();
    printf("dwcnt - 32 bit incrementing count.\n"); pause();
    printf("val   - Numeric 32 bit value, big endian.\n"); pause();
    printf("rand  - Random byte value.\n"); pause();
    printf("lba   - Only the first 32 bits get LBA, rest is $ff. LBA starts \n"); pause();
    printf("        at [val], and increments across buffer. Note that this only\n"); pause();
    printf("        writes the first dword of each sector, use another pattern\n"); pause();
    printf("        to fill the background.\n"); pause();
    printf("buffs - Compare the read and write buffers to each other. This allows\n"); pause();
    printf("        complex patterns to be built up in the write buffer.\n"); pause();
    printf("\n"); pause();
    printf("All write operations are from the write buffer which is %d sectors long.\n", NOSECS); pause();
    printf("All read operations are from the read buffer which is %d sectors long.\n", NOSECS); pause();
    printf("\n"); pause();
    printf("All drives start write locked, and are relocked when the drive is changed.\n"); pause();
    printf("\n"); pause();
    printf("User variables start with a-z and continue with a-z and 0-9 like Myvar1.\n"); pause();
    printf("They are created or recreated by set, and can be set any number of times.\n"); pause();
    printf("A variable can be used anywhere a val can.\n"); pause();
    printf("\n"); pause();
    printf("There are several predefined variables:\n"); pause();
    printf("\n"); pause();
    printf("drvsiz - Gives the size of the current physical drive.\n"); pause();
    printf("rand   - Gives a random number.\n"); pause();
    printf("lbarnd - Gives a random LBA for the current drive, ie., a random number\n"); pause();
    printf("         that fits into 0..drvsiz-1.\n"); pause();
    printf("secsiz - Size of sector in bytes (always 512).\n"); pause();
    printf("bufsiz - Size of read and write buffers in sectors.\n"); pause();
    printf("\n"); pause();
    printf("The compare modes are:\n"); pause();
    printf("\n"); pause();
    printf("all - Show all mismatches.\n"); pause();
    printf("one - Show only the first mismatch.\n"); pause();
    printf("fail - Fail (abort) after the first mismatch (normal is continue).\n"); pause();
    printf("\n"); pause();
    printf("All numeric parameters can be expressions, using C style expression operators\n"); pause();
    printf("+a,-a,(a),a*b,a/b,a%%b,a+b,a-b,a<b,a>b,a=b, a!=b,a<=b and a>b.\n"); pause();
    printf("Note that expressions cannot contain spaces, ie., a numeric parameter cannot\n"); pause();
    printf("have spaces within it.\n"); pause();
    printf("\n"); pause();
    printf("Format strings used with print must start and end with double quotes.\n"); pause();
    printf("They can contain standard C style format specifiers like %%[w[.p]f, where\n"); pause();
    printf("the format character is d, x, or o, for decimal, hexadecimal or octal.\n"); pause();
    printf("The sign and length cannot be specified, since debug values are always signed\n"); pause();
    printf("long long values. Anything else in the format string is printed. There are\n"); pause();
    printf("no character escapes. Use pn/printn and p/print to specify or leave out a\n");  pause();
    printf("newline after printout.\n"); pause();
    printf("\n"); pause();
    printf("Note that leading zeros don't work in the field width. Use the precision\n"); pause();
    printf("instead, i.e., %%4.4x not %%04x.\n"); pause();
    printf("\n"); pause();
    printf("Stored program lines are entered with a leading number as:\n"); pause();
    printf("\n"); pause();
    printf("Diag> 1 dothis(num): echon The number is: ; p num\n"); pause();
    printf("\n"); pause();
    printf("The line is inserted BEFORE the line in the program.\n"); pause();
    printf("\n"); pause();
    printf("Any line with a label (as \"dothis:\" above) can be called as a\n"); pause();
    printf("procedure by using it's label as a command as:\n"); pause();
    printf("\n"); pause();
    printf("Diag> dothis 42\n"); pause();
    printf("\n"); pause();
    printf("Note parameters are optional.\n"); pause();
    printf("\n"); pause();
    printf("Procedure execution stops with an \"end\" command, and execution\n"); pause();
    printf("resumes after the calling command.\n"); pause();
    printf("\n"); pause();
    printf("All variables created in a procedure are temporary and removed at\n"); pause();
    printf("the end of the procedure. Variables are only created in a procedure\n"); pause();
    printf("if they don't exist outside of the procedure OR if \"local\" is used.\n"); pause();
    printf("Note that parameters are automatically local.\n"); pause();
    printf("\n"); pause();
    printf("Note that hitting the end of the program buffer terminates the run\n"); pause();
    printf("ALWAYS.\n"); pause();
    printf("\n"); pause();
    printf("The file \"discdiag.ini\", if present in the current directory, is\n"); pause();
    printf("automatically loaded when discdiag starts.\n"); pause();
    printf("\n"); pause();
    printf("*** WARNING: This diagnostic CAN and WILL distroy your hard disc!\n"); pause();
    printf("\n"); pause();

    return result_ok; // return no fault
    
}

/**
 *
 * Read sector
 *
 * Read sector to buffer
 *
 * \returns Standard discdiag error code.
 * 
 */

result command_read(
    /** Remaining command line */ char **line
)

{

    long long lba; // lba to read
    long long numsecs; // number of sectors to read
    long long v;
    result r;
    int nr;
    
    lba = 0; // set default lba
    numsecs = 1; // set default number of sectors
    while (**line == ' ') (*line)++; // skip any leading spaces
    if (**line && **line != ';') { // get lba number

        r = getparam(line, &v);
        lba = v;
        if (r != result_ok) return r;
        if (**line && **line != ';') { // get number of sectors

            r = getparam(line, &v);
            numsecs = v;
            if (r != result_ok) return r;

        } 
        
    }
    // validate drive is active
    if (currentdrive < 0) {

        printf("*** Error: No current drive is set\n");

        return result_error;

    }
    // validate sector count is within buffer
    if (numsecs > NOSECS) {

        printf("*** Error: Invalid sector count, must be <= %d\n", NOSECS);

        return result_error;

    }
    // validate lba is 0 to drive size
    if (lba >= drivesize) {

        printf("*** Error: Invalid lba number, must be <= %lld\n", drivesize);

        return result_error;

    }
    // validate lba+sectors are within drive
    if (lba+numsecs-1 >= drivesize) {

        printf("*** Error: Operation will exceed drive size\n");

        return result_error;

    }

    /* read sector to buffer */
    nr = readsector(readbuffer, lba, numsecs);
    if (nr) {

        printf("*** Error: Read error\n");

        return result_error; // read failed, exit

    }

    // update statistics
    iopread += 1.0; // read IOPs
    bcread += numsecs*SECSIZE; // read bytes
 
    return result_ok; // return no fault
   
}

/**
 *
 * Write sector
 *
 * Write sector from buffer
 *
 * \returns Standard discdiag error code.
 * 
 */

result command_write(
    /** Remaining command line */ char **line
)

{

    long long lba; // lba to read
    long long numsecs; // number of sectors to read
    result r;
    long long v;
    int nr;
    
    if (writeprot) {

        printf("*** Error: Drive is write protected, use unprot command\n");
        return result_error;

    }

    lba = 0; // set default lba
    numsecs = 1; // set default number of sectors
    while (**line == ' ') (*line)++; // skip any leading spaces
    if (**line && **line != ';') { // get lba number

        r = getparam(line, &v);
        lba = v;
        if (r != result_ok) return r;
        if (**line && **line != ';') { // get number of sectors

            r = getparam(line, &v);
            numsecs = v;
            if (r != result_ok) return r;

        } 
        
    }
    // validate drive is active
    if (currentdrive < 0) {

        printf("*** Error: No current drive is set\n");

        return result_error;

    }
    // validate sector count is within buffer
    if (numsecs > NOSECS) {

        printf("*** Error: Invalid sector count, must be <= %d\n", NOSECS);
        return result_error;

    }
    // validate lba is 0 to drive size
    if (lba >= drivesize) {

        printf("*** Error: Invalid lba number, must be <= %lld\n", drivesize);
        return result_error;

    }
    // validate lba+sectors are within drive
    if (lba+numsecs-1 >= drivesize) {

        printf("*** Error: Operation will exceed drive size\n");

        return result_error;

    }

    /* write sector from buffer */
    nr = writesector(writebuffer, lba, numsecs);
    if (nr) {

        printf("*** Error: Write error\n");

        return result_error; // read failed, exit

    }
 
    // update statistics
    iopwrite += 1.0; // write IOPs
    bcwrite += numsecs*SECSIZE; // write bytes

    return result_ok; // return no fault
   
}
 
/**
 *
 * Dump write sectors
 *
 * Dump sectors in write buffer.
 *
 * \returns Standard discdiag error code.
 * 
 */

result command_dumpwrite(
    /** Remaining command line */ char **line
)

{

    long long numsecs; // number of sectors to read
    result r;
    long long v;

    numsecs = 1; // set default number of sectors
    while (**line == ' ') (*line)++; // skip any leading spaces
    if (**line && **line != ';') { // get number of sectors

        r = getparam(line, &v);
        numsecs = v;
        if (r != result_ok) return r;

    } 
    if (numsecs > NOSECS) {

        printf("*** Error: Invalid sector count, must be <= %d\n", NOSECS);
        return result_error;

    }

    /* dump sector in buffer in hex and ASCII */
    printf("Contents of sector:\n");
    printf("\n");
    r = dump(writebuffer, SECSIZE*numsecs);
    if (r != result_ok) return r;
    printf("\n");
 
    return result_ok; // return no fault
   
}

/**
 *
 * Dump read sectors
 *
 * Dump sectors in read buffer.
 *
 * \returns Standard discdiag error code.
 * 
 */

result command_dumpread(
    /** Remaining command line */ char **line
)

{

    long long numsecs; // number of sectors to read
    result r;
    long long v;

    numsecs = 1; // set default number of sectors
    while (**line == ' ') (*line)++; // skip any leading spaces
    if (**line && **line != ';') { // get number of sectors

        r = getparam(line, &v);
        numsecs = v;
        if (r != result_ok) return r;

    } 

    if (numsecs > NOSECS) {

        printf("*** Error: Invalid sector count, must be <= %d\n", NOSECS);
        return result_error;

    }

    /* dump sector in buffer in hex and ASCII */
    printf("Contents of sector:\n");
    printf("\n");
    r = dump(readbuffer, SECSIZE*numsecs);
    if (r != result_ok) return r;
    printf("\n");
 
    return result_ok; // return no fault
   
}

/**
 *
 * Set pattern
 *
 * Set pattern for the write sector buffer. The patterns available are:
 *
 * cnt   - Byte incrementing count.
 * dwcnt - 32 bit incrementing count.
 * val   - Numeric 32 bit value, big endian.
 * rand  - Random byte value.
 * lba   - Only the first 32 bits get LBA, rest is $ff. LBA starts
 *         at [val], and increments across buffer. Note that this only writes
 *         the first dword of each sector.
 *
 * The command format is:
 *
 *    pattn [type] [val]
 *
 * The type is the name of the pattern from above. The val is numeric and
 * is only used for the val and lba patterns, and ignored otherwise.
 * 
 * \returns Standard discdiag error code.
 * 
 */

result command_pattn(
    /** Remaining command line */ char **line
)

{

    char pat[100]; // pattern name
    long long val; // value
    long long len; // length in sectors
    unsigned long seeds; // save for random seed
    unsigned long l;
    long i, s;
    result r;

    seeds = seed; // save the random seed
    seed = 42; // reset random number generator

    strcpy(pat, "cnt"); // set default pattern is byte count
    val = 0; // set default value
    len = NOSECS; // set length is full buffer
    while (**line == ' ') (*line)++; // skip any leading spaces
    if (**line && **line != ';') { // get pattern name

        getword(line, pat); // get pattern name
        if (**line && **line != ';') { // get value

            r = getparam(line, &val);
            if (r != result_ok) {

                seed = seeds; // restore the random seed

                return r;

            }
            while (**line == ' ') (*line)++; // skip any leading spaces
            if (**line && **line != ';') { // get length in sectors

                r = getparam(line, &len);
                if (r != result_ok) {

                    seed = seeds; // restore the random seed

                    return r;

                }

            }

        } 
        
    }

    if (!strcmp(pat, "cnt")) {

        for (i = 0; i < SECSIZE*len; i++) writebuffer[i] = i & 0xff;

    } else if (!strcmp(pat, "dwcnt")) {

        l = 0;
        for (i = 0; i < SECSIZE*len; i += 4) {

            writebuffer[i] = l >> 24 & 0xff;
            writebuffer[i+1] = l >> 16 & 0xff;
            writebuffer[i+2] = l >> 8 & 0xff;
            writebuffer[i+3] = l & 0xff;
            l++;

        }

    } else if (!strcmp(pat, "val")) {

        for (i = 0; i < SECSIZE*len; i += 4) {

            writebuffer[i] = val >> 24 & 0xff;
            writebuffer[i+1] = val >> 16 & 0xff;
            writebuffer[i+2] = val >> 8 & 0xff;
            writebuffer[i+3] = val & 0xff;

        }

    } else if (!strcmp(pat, "rand")) {

        for (s = 0; s < len; s++) { // sectors

            // the random pattern needs to be the same for each sector
            seed = 42; // reset random number generator
            for (i = 0; i < SECSIZE; i++) writebuffer[s*SECSIZE+i] = rand64() & 0xff;

        }

    } else if (!strcmp(pat, "lba")) {

        for (i = 0; i < SECSIZE*len; i += SECSIZE) {

            writebuffer[i] = val >> 24 & 0xff;
            writebuffer[i+1] = val >> 16 & 0xff;
            writebuffer[i+2] = val >> 8 & 0xff;
            writebuffer[i+3] = val & 0xff;
            val++;

        }

    } else {

        printf("*** Error: bad pattern name: %s\n", pat);
        seed = seeds; // restore the random seed

        return result_error;

    }
    seed = seeds; // restore the random seed

    return result_ok;

}

/**
 *
 * Compare pattern
 *
 * Compare pattern for the read sector buffer. The patterns available are:
 *
 * cnt   - Byte incrementing count.
 * dwcnt - 32 bit incrementing count.
 * val   - Numeric 32 bit value, big endian.
 * rand  - Random byte value.
 * lba   - Only the first 32 bits get LBA, rest is $ff. LBA starts
 *         at [val], and increments across buffer. Note that this only writes
 *         the first dword of each sector.
 *
 * The command format is:
 *
 *    comp [type] [val]
 *
 * The type is the name of the pattern from above. The val is numeric and
 * is only used for the val and lba patterns, and ignored otherwise.
 *
 * comp is the direct opposite of pattn. It verifies that the pattern set up
 * by pattn exists in the buffer, and verifies in the read buffer instead of
 * the write buffer.
 * 
 * \returns Standard discdiag error code.
 * 
 */

result command_comp(
    /** Remaining command line */ char **line
)

{

    char pat[100]; // pattern name
    long long val; // value
    long long len; // length in sectors
    unsigned long seeds; // save for random seed
    unsigned long l;
    long i, s;
    result r;
    
    seeds = seed; // save the random seed
    seed = 42; // reset random number generator

    strcpy(pat, "cnt"); // set default pattern is byte count
    val = 0; // set default value
    len = NOSECS; // set length is full buffer
    first = 1; // set first miscompare
    dataset = 0; // last data not set
    repcnt = 0; // clear mismatch count
    while (**line == ' ') (*line)++; // skip any leading spaces
    if (**line && **line != ';') { // get pattern name

        getword(line, pat); // get pattern name
        while (**line == ' ') (*line)++; // skip any leading spaces
        if (**line && **line != ';') { // get value

            r = getparam(line, &val);
            if (r != result_ok) {

                seed = seeds; // restore the random seed

                return r;

            }
            while (**line == ' ') (*line)++; // skip any leading spaces
            if (**line && **line != ';') { // get length in sectors

                r = getparam(line, &len);
                if (r != result_ok) {

                    seed = seeds; // restore the random seed

                    return r;

                }

            }

        } 
        
    }
    if (!strcmp(pat, "cnt")) {

        for (i = 0; i < SECSIZE*len; i++) {

            r = printcomp(i, readbuffer[i], i & 0xff);
            if (r != result_ok) {

                seed = seeds; // restore the random seed

                return r;

            }

        }

    } else if (!strcmp(pat, "dwcnt")) {

        l = 0;
        for (i = 0; i < SECSIZE*len; i += 4) {

            r = printcomp(i, readbuffer[i], l >> 24 & 0xff);
            if (r != result_ok) {

                seed = seeds; // restore the random seed

                return r;

            }
            r = printcomp(i+1, readbuffer[i+1], l >> 16 & 0xff);
            if (r != result_ok) {

                seed = seeds; // restore the random seed

                return r;

            }
            r = printcomp(i+2, readbuffer[i+2], l >> 8 & 0xff);
            if (r != result_ok) {

                seed = seeds; // restore the random seed

                return r;

                
            }
            r = printcomp(i+3, readbuffer[i+3], l & 0xff);
            if (r != result_ok) {

                seed = seeds; // restore the random seed

                return r;

            }
            l++;

        }

    } else if (!strcmp(pat, "val")) {

        for (i = 0; i < SECSIZE*len; i += 4) {

            r = printcomp(i, readbuffer[i], val >> 24 & 0xff);
            if (r != result_ok) {

                seed = seeds; // restore the random seed

                return r;

            }
            r = printcomp(i+1, readbuffer[i+1], val >> 16 & 0xff);
            if (r != result_ok) {

                seed = seeds; // restore the random seed

                return r;

            }
            r = printcomp(i+2, readbuffer[i+2], val >> 8 & 0xff);
            if (r != result_ok) {

                seed = seeds; // restore the random seed

                return r;

            }
            r = printcomp(i+3, readbuffer[i+3], val & 0xff);
            if (r != result_ok) {

                seed = seeds; // restore the random seed

                return r;

            }  

        }

    } else if (!strcmp(pat, "rand")) {

        for (s = 0; s < len; s++) { // sectors

            // the random pattern needs to be the same for each sector
            seed = 42; // reset random number generator
            for (i = 0; i < SECSIZE; i++) {

                r = printcomp(i, readbuffer[s*SECSIZE+i], rand64() & 0xff);
                if (r != result_ok) {

                    seed = seeds; // restore the random seed

                    return r;

                }

            }

        }

    } else if (!strcmp(pat, "lba")) {

        for (i = 0; i < SECSIZE*len; i += SECSIZE) {

            r = printcomp(i, readbuffer[i], val >> 24 & 0xff);
            if (r != result_ok) {

                seed = seeds; // restore the random seed

                return r;

            }
            r = printcomp(i+1, readbuffer[i+1], val >> 16 & 0xff);
            if (r != result_ok) {

                seed = seeds; // restore the random seed

                return r;

            }
            r = printcomp(i+2, readbuffer[i+2], val >> 8 & 0xff);
            if (r != result_ok) {

                seed = seeds; // restore the random seed

                return r;

            }
            r = printcomp(i+3, readbuffer[i+3], val & 0xff);
            if (r != result_ok) {

                seed = seeds; // restore the random seed

                return r;

            }
            val++;

        }

    } else if (!strcmp(pat, "buffs")) {

        for (i = 0; i < SECSIZE*len; i++) {

            r = printcomp(i, readbuffer[i], writebuffer[i]);
            if (r != result_ok) {

                seed = seeds; // restore the random seed

                return r;

            }

        }

    } else {

        printf("*** Error: bad pattern name: %s\n", pat);
        seed = seeds; // restore the random seed

        return result_error;

    }
    // message if miscompares have accumulated
    if (repcnt) {

        printf("**** Info: There were %d occurrances of the above mismatch\n", repcnt);
        repcnt = 0; // reset counter

    }
    seed = seeds; // restore the random seed

    return result_ok;

}

/**
 *
 * Set compare mode
 *
 * Set the compare mode to one of the following
 *
 * all - Print all mismatches.
 * one - Prints only the first mismatch.
 * fail - Fails when the first mismatch is seen.
 *
 * Note that the first two modes do not stop the program.
 * 
 * \returns Standard discdiag error code.
 * 
 */

result command_compmode(
    /** Remaining command line */ char **line
)

{

    char w[100]; // word buffer
    
    getword(line, w); // get mode name
    if (!strcmp(w, "all")) curmode = compmode_all; // all
    else if (!strcmp(w, "one")) curmode = compmode_one; // one
    else if (!strcmp(w, "fail")) curmode = compmode_fail; // fail
    else {

        printf("*** Error: mode not recognized\n");

        return result_error;

    }

    return result_ok;

}

/**
 *
 * Set physical drive to access
 *
 * Sets the current physical drive to access. Also resets the write protect back
 * to "on" by default. Also outputs a warning for accesses of the 0 drive, which
 * is generally the system drive.
 *
 * \returns Standard discdiag error code.
 * 
 */

result command_drive(
    /** Remaining command line */ char **line
)

{

    result r;
    long long v;
    int ri;
    int drive;
    long long t;

    while (**line == ' ') (*line)++; // skip any leading spaces
    if (**line && **line != ';') { // get drive parameter

        r = getparam(line, &v); // get drive number
        if (r != result_ok) return r;
        drive = (int) v;
        writeprot = 1; // set the write protect on the new drive by default
        if (!drive) printf("*** Warning: You have selected the system drive\n");
        ri = setdrive(drive); // set physical drive active
        if (ri) return result_error; // error
        currentdrive = drive; // set that active
        // get and store current drive size
        r = physize(&t);
        if (r != 0) return result_error;
        drivesize = t / SECSIZE; // find net size in sectors
        if (t % SECSIZE) {
        
            printf("*** Warning: Drive total size is not an even number of sectors\n");
        
        }
        // clear the statistics on this drive
        iopwrite = 0.0;
        iopread = 0.0;
        bcwrite = 0.0;
        bcread = 0.0;

    } else {

        if (getdrive() < 0)
            printf("Current drive is: Not set\n");
        else
            printf("Current drive is: %d\n", getdrive());

    }

    return result_ok; // return no exit
   
}

/**
 *
 * List physical drives available
 *
 * Lists what physical drives are currently available.
 *
 * \returns Standard discdiag error code.
 * 
 */

result command_listdrives(
    /** Remaining command line */ char **line
)

{

    int i, r;
    long long s, t;

    printf("Physical drives available:\n");
    printf("\n");
    for (i = 0; i < 10; i++) {

        r = testdrive(i); // test this drive
        if (!r) {

            r = testsize(i, &t); // get the drive size
            if (!r) {

                s = t / SECSIZE; // find net size in sectors
                printf("Drive %d (%s) available %lld lbas\n", i, getdrvstr(i), s);

            }

        }

    }
    printf("\n");

    return result_ok; // return no exit
   
}

/**
 *
 * Unprotect the current drive
 *
 * Resets the unprotect flag. We are fairly paranoid about overwriting one of
 * the system drives, so this flag gets set on at the start, and anytime we
 * change drives.
 *
 * \returns Standard discdiag error code.
 * 
 */

result command_unprot(
    /** Remaining command line */ char **line
)

{

    writeprot = 0; // Turn off write protect

    return result_ok; // return no exit
   
}

/**
 *
 * Echo command line text
 *
 * Simply prints the text up to either the end of the line or to the ';'
 * signifying the next command.
 * Unprotect the current drive
 *
 * Resets the unprotect flag. We are fairly paranoid about overwriting one of
 * the system drives, so this flag gets set on at the start, and anytime we
 * change drives.
 *
 * \returns Standard discdiag error code.
 * 
 */

result command_echon(
    /** Remaining command line */ char **line
)

{

    while (**line == ' ') *(*line)++; // skip leading spaces
    // print the rest of the parameter area
    while (**line && **line != ';') printf("%c", *(*line)++);

    return result_ok; // return no exit
   
}

/**
 *
 * Echo command line text
 *
 * Simply prints the text up to either the end of the line or to the ';'
 * signifying the next command.
 * Unprotect the current drive
 *
 * Resets the unprotect flag. We are fairly paranoid about overwriting one of
 * the system drives, so this flag gets set on at the start, and anytime we
 * change drives.
 *
 * \returns Standard discdiag error code.
 * 
 */

result command_echo(
    /** Remaining command line */ char **line
)

{

    result r; // result value

    r = command_echon(line);
    if (r == result_ok) printf("\n");

    return r; // return result
   
}

/**
 *
 * Loop on command line
 *
 * Loops from the beginning of the command line. The command form is:
 *
 * loop [num]
 *
 * If the number is present, then the loop counter is compared to the parameter
 * and the loop is not repeated if the loop counter is greater than or equal.
 * If parameter is present, the loop always takes place.
 *
 * The loop is done by simply reseting the parsing to the start of the command
 * line. The count is printed.
 *
 * \returns Standard discdiag error code.
 * 
 */

result command_loop(
    /** Remaining command line */ char **line
)

{

    int stopcount; 
    result r;
    long long v;
    loopcounter* cp;

    stopcount = -1; // default is loop forever
    while (**line == ' ') (*line)++; // skip any leading spaces
    if (**line && **line != ';') { // get loop stop count

        r = getparam(line, &v);
        stopcount = (int) v;
        if (r != result_ok) return r;

    }
    if (introot) {

        // find or create line counter entry
        cp = fndcnt(&introot->curlin->looplist, *line);
        cp->loopcount++; // increment loop count
        printf("Iteration: %d\n", cp->loopcount);
        if (stopcount < 0 || cp->loopcount < stopcount) {

            *line = introot->curlin->line; // reset line parsing
            introot->curchr = *line;
            return result_restart; // flag restart line

        } else cp->loopcount = 0; // reset counter

    }

    return result_ok; // return result ok
   
}

/**
 *
 * Loop on command line quietly
 *
 * Loops from the beginning of the command line. The command form is:
 *
 * loopq [num]
 *
 * If the number is present, then the loop counter is compared to the parameter
 * and the loop is not repeated if the loop counter is greater than or equal.
 * If parameter is present, the loop always takes place.
 *
 * The loop is done by simply reseting the parsing to the start of the command
 * line. The count is NOT printed.
 *
 * \returns Standard discdiag error code.
 * 
 */

result command_loopq(
    /** Remaining command line */ char **line
)

{

    int stopcount; 
    result r;
    long long v;
    loopcounter* cp;

    stopcount = -1; // default is loop forever
    while (**line == ' ') (*line)++; // skip any leading spaces
    if (**line && **line != ';') { // get loop stop count

        r = getparam(line, &v);
        stopcount = (int) v;
        if (r != result_ok) return r;

    }
    if (introot) {

        // find or create line counter entry
        cp = fndcnt(&introot->curlin->looplist, *line);
        cp->loopcount++; // increment loop count
        if (stopcount < 0 || cp->loopcount < stopcount) {

            *line = introot->curlin->line; // reset line parsing
            introot->curchr = *line;
            return result_restart; // flag restart line

        } else cp->loopcount = 0; // reset counter

    }

    return result_ok; // return result ok
   
}

/**
 *
 * Loop until
 *
 * Loop until value true
 *
 * Loops from the beginning of the command line. The command form is:
 *
 * u val
 *
 * If the value is true, the loop stops, otherwise the command line is 
 * restarted.
 *
 * \returns Standard discdiag error code.
 * 
 */

result command_untill(
    /** Remaining command line */ char **line
)

{

    result r;
    long long v;

    r = getparam(line, &v);
    if (r != result_ok) return r;
    if (!v) { // condition not met, repeat line

        // The interp stack should not be dry, just kick it to the error handler
        // in poplvl.
        if (!introot) poplvl();
        *line = introot->curlin->line; // reset line parsing
        return result_restart; // flag restart line

    }

    return result_ok; // return result ok
   
}

/**
 *
 * While
 *
 * Loop until the given condition is true.
 *
 * \returns Standard discdiag error code.
 * 
 */

result command_while(
    /** Remaining command line */ char **line
)

{

    result r;
    long long v;
    ctlstk *cp;
    char *lines;
    int what;

    lines = *line; // save current location on line
    r = getparam(line, &v);
    if (r != result_ok) return r;
    if (!v) { // condition not met, advance to wend

        skipcmd(line, "wend", "", "", &what); // skip to nearest wend

    } else {

        // condition met, throw control frame and continue
        cp = (ctlstk *) malloc(sizeof(ctlstk));
        cp->next = ctlroot; // push onto controls stack
        ctlroot = cp;
        cp->linpos = introot->curlin; // place line
        cp->chrpos = lines; // set repeat back to parameter
        cp->ctl = ctl_while;

    }

    return result_ok; // return result ok
   
}

/**
 *
 * While end
 *
 * Find the original while and repeat it.
 *
 * Note we only hit the wend if the original while loop executed the body.
 *
 * \returns Standard discdiag error code.
 * 
 */

result command_wend(
    /** Remaining command line */ char **line
)

{

    linestr *curlins;
    char *curchrs;
    char *lines;
    result r;
    long long v;

    // purge the control stack until a while is seen or empty
    if (ctlroot && ctlroot->ctl != ctl_while) popctl();
    if (!ctlroot) {

        printf("*** Error: No \"while\" is active\n");

        return result_error;

    }
    if (introot) {

        // save current position past wend
        curlins = introot->curlin;
        curchrs = introot->curchr;
        lines = *line;
        // restore back to parameter start
        introot->curlin = ctlroot->linpos;
        introot->curchr = ctlroot->chrpos;
        *line = introot->curchr;
        r = getparam(line, &v); // get parameter again
        if (r != result_ok) return r; // error
        if (!v) { // condition not met, advance to wend

            introot->curlin = curlins;
            introot->curchr = curchrs;
            *line = lines;
            popctl(); // remove the control frame

        }

    }

    return result_ok; // return result ok
   
}

/**
 *
 * Repeat
 *
 * Marks the beginning of a repeat block.
 *
 * \returns Standard discdiag error code.
 * 
 */

result command_repeat(
    /** Remaining command line */ char **line
)

{

    ctlstk *cp;

    // throw control frame and continue
    cp = (ctlstk *) malloc(sizeof(ctlstk));
    cp->next = ctlroot; // push onto controls stack
    ctlroot = cp;
    cp->linpos = introot->curlin; // place line
    cp->chrpos = *line; // set repeat back to parameter
    cp->ctl = ctl_repeat;

    return result_ok; // return result ok
   
}

/**
 *
 * Until
 *
 * Ends a repeat block. Evaluates a condition, and goes back to the repeat
 * start if true.
 *
 * \returns Standard discdiag error code.
 * 
 */

result command_until(
    /** Remaining command line */ char **line
)

{

    result r;
    long long v;

    // purge the control stack until a repeat is seen or empty
    if (ctlroot && ctlroot->ctl != ctl_repeat) popctl();
    if (!ctlroot) {

        printf("*** Error: No \"repeat\" is active\n");

        return result_error;

    }
    r = getparam(line, &v);
    if (r != result_ok) return r;
    if (!v) { // condition not met, return to repeat

        introot->curlin = ctlroot->linpos;
        introot->curchr = ctlroot->chrpos;
        *line = introot->curchr;

    } else popctl(); // remove control frame and continue

    return result_ok; // return result ok
   
}

/**
 *
 * For loop
 *
 * Runs a for loop. The command format is:
 *
 * for var start end
 *
 * The variable is loaded with the start value, then checked as below or
 * equal to the end value. If so, the loop body is run, otherwise we skip
 * to the next fend command.
 *
 * \returns Standard discdiag error code.
 * 
 */
result command_for(
    /** Remaining command line */ char **line
)

{

    char w[100]; // buffer for parameter
    uservar *uvar; // pointer to user variable entry
    char *lines;
    long long s, e, st;
    result r;
    ctlstk *cp;
    int what;

    st = 1; // set default step
    getword(line, w); // get name of variable
    // get value for start
    r = getparam(line, &s);
    if (r != result_ok) return r;
    lines = *line; // save current location on line (before end val)
    // get value for end
    r = getparam(line, &e);
    if (r != result_ok) return r;
    // check step exists
    while (**line == ' ') (*line)++; // skip any leading spaces
    if (**line && **line != ';') { // get step parameter

        r = getparam(line, &st); // get step parameter
        if (r != result_ok) return r;

    }

    // try searching the user variables list
    uvar = fndvar(w);
    if (uvar) { // found

        // place value of start
        uvar->val = s;

    } else { // not found

        r = pushvar(w, s); // enter new variable
        if (r != result_ok) return r; // error
        uvar = fndvar(w); // get it again

    }

    // if the value is outside the range, quit
    if ((s > e && st >= 0) || (s < e && st <0)) 
        skipcmd(line, "fend", "", "", &what);
    else {

        // condition met, throw control frame and continue
        cp = (ctlstk *) malloc(sizeof(ctlstk));
        cp->next = ctlroot; // push onto controls stack
        ctlroot = cp;
        cp->linpos = introot->curlin; // place line
        cp->chrpos = lines; // set repeat back to parameter
        cp->ctl = ctl_for; // place control type
        cp->var = uvar; // place variable
        cp->step = st; // place step

    }

    return result_ok; // return result ok

}

/**
 *
 * End for loop
 *
 * Returns to the top of the for loop, then checks that the variable is still
 * less than or equal to the end value. If so, we return to the top and go
 * again, otherwise the loop is exited.
 *
 * \returns Standard discdiag error code.
 * 
 */
result command_fend(
    /** Remaining command line */ char **line
)

{

    linestr *curlins;
    char *curchrs;
    char *lines;
    result r;
    long long s, e, st;

    // purge the control stack until a for is seen or empty
    if (ctlroot && ctlroot->ctl != ctl_for) popctl();
    if (!ctlroot) {

        printf("*** Error: No \"for\" is active\n");

        return result_error;

    }
    st = ctlroot->step; // get step
    // increment for value and fetch
    s = ctlroot->var->val = ctlroot->var->val+st;
    if (introot) {

        // save current position past fend
        curlins = introot->curlin;
        curchrs = introot->curchr;
        lines = *line;
        // restore back to end parameter start
        introot->curlin = ctlroot->linpos;
        introot->curchr = ctlroot->chrpos;
        *line = introot->curchr;
        r = getparam(line, &e); // get end parameter again
        if (r != result_ok) return r; // error
        // skip possible step parameter
        while (**line && **line != ';') (*line)++;
        if ((s > e && st >= 0) || (s < e && st <0)) { // out of range, advance to fend

            introot->curlin = curlins;
            introot->curchr = curchrs;
            *line = lines;
            popctl(); // remove the control frame

        }

    }

    return result_ok; // return result ok

}

/**
 *
 * Select case value
 *
 * Runs a case select. The format of the command is;
 *
 * select val
 *
 * The value is picked up and matched to all case statement values seen.
 * If the send command is seen, we terminate.
 *
 * \returns Standard discdiag error code.
 * 
 */
result command_select(
    /** Remaining command line */ char **line
)

{

    long long v, m;
    int what;
    int found;
    result r;
    char c;

    // get selector value
    r = getparam(line, &v);
    if (r != result_ok) return r;
    found = 0; // set not found
    do { // examine cases

        // look for case or send
        r = skipcmd(line, "case", "default", "send", &what);
        if (r != result_ok) return r;
        if (what == 1) { // process case match

            do { // examine each presented case
            
                // get match value
                r = getparam(line, &m);
                if (r != result_ok) return r;
                if (v == m) found = 1; // set found status
                while (**line == ' ') (*line)++; // skip any spaces
                
            } while (**line && **line != ';'); // not end of command
     
        } else if (what == 2) found = 1; // default matches all      

    } while (!found && what == 1); // until match found or send

    return result_ok; // return result ok

}

/**
 *
 * Case value
 *
 * Since the select command runs the select operation, a case command seen
 * alone simply marks the end of a case sequence. We advance to the next send
 * and resume.
 *
 * \returns Standard discdiag error code.
 * 
 */
result command_case(
    /** Remaining command line */ char **line
)

{

    int what;
    result r;

    // search for send
    r = skipcmd(line, "send", "", "", &what);
    if (r != result_ok) return r;

    return result_ok; // return result ok

}

/**
 *
 * Default case
 *
 * Since the select command runs the select operation, a default case command 
 * seen alone simply marks the end of a case sequence. We advance to the next 
 * send and resume.
 *
 * \returns Standard discdiag error code.
 * 
 */
result command_default(
    /** Remaining command line */ char **line
)

{

    int what;
    result r;

    // search for send
    r = skipcmd(line, "send", "", "", &what);
    if (r != result_ok) return r;

    return result_ok; // return result ok

}

/**
 *
 * Select end
 *
 * Since the select command runs the select operation, a send just marks the end
 * of the select series. Thus it is effectively a no-op.
 *
 * \returns Standard discdiag error code.
 * 
 */
result command_send(
    /** Remaining command line */ char **line
)

{

    return result_ok; // return result ok

}

/**
 *
 * Print value
 *
 * Prints any value or variable.
 *
 * \returns Standard discdiag error code.
 * 
 */

result command_printn(
    /** Remaining command line */ char **line
)

{

    result r;
    long long v;
    char c;
    char fmt[100];
    int i;
    int w, p;
    int vv;

    fmt[0] = 0; // set default no format
    while (**line == ' ') (*line)++; // skip spaces
    if (**line == '"') { // get a format string

        (*line)++; // skip leading quote
        i = 0;
        while (**line != '"' && **line) {

            if (**line == '\\' && *((*line)+1)) (*line)++; // force next character
            fmt[i++] = **line; // get string character
            (*line)++; // next

        }
        if (**line != '"') {

            printf("*** Error: Unterminated format string\n");

            return result_error;

        }
        (*line)++; // skip trailing quote
        fmt[i] = 0; // terminate format string
       
    }
    i = 0; // reset string
    // if not end of command, process parameters
    do {

        // output any filler characters
        while (fmt[i] && fmt[i] != '%') printf("%c", fmt[i++]);
        v = 0; // clear default value
        vv = 0;
        while (**line == ' ') (*line)++; // skip any leading spaces
        if (**line && **line != ';') { // not end of line

            r = getparam(line, &v);
            if (r != result_ok) return r;
            vv = 1;

        }
        if (fmt[i] == '%') { // there is a format character

            i++; // skip '%'
            w = 1; // default width
            p = 1; // default precision
            // find width
            if (fmt[i] >= '0' && fmt[i] <= '9') {

                w = 0; // clear width
                while (fmt[i] >= '0' && fmt[i] <= '9') w = w*10+fmt[i++]-'0';

            }
            if (fmt[i] == '.') { // there is a precision

                i++; // skip '.'
                if (fmt[i] >= '0' && fmt[i] <= '9') {

                    p = 0; // clear precision
                    while (fmt[i] >= '0' && fmt[i] <= '9') p = p*10+fmt[i++]-'0';

                }

            }
            switch (fmt[i]) { // execute format command

                // since all we have is a number, and its length and sign are
                // predetermined, all that can be done is to select the base
                // to output. If the format character is not recognized,
                // we use the default decimal.
                case 'd': printf("%*.*lld", w, p, v); i++; break;
                case 'x': printf("%*.*llx", w, p, v); i++; break;
                case 'o': printf("%*.*llo", w, p, v); i++; break;
                default: printf("%lld", v); break;
                
            }

        } else if (vv) printf("%lld ", v); // use default format

    } while (**line && **line != ';'); // not end of command

    return result_ok; // return result ok

}

/**
 *
 * Print value with newline
 *
 * Prints any value or variable with trailing newline.
 *
 * \returns Standard discdiag error code.
 * 
 */

result command_print(
    /** Remaining command line */ char **line
)

{

    result r;
    long long v;
    char c;

    // execute the no line-feed print
    r = command_printn(line);
    printf("\n"); // terminate

    return r; // return result

}

/**
 *
 * Set user variable
 *
 * Sets either a new user variable or resets an existing one.
 *
 * \returns Standard discdiag error code.
 * 
 */

result command_set(
    /** Remaining command line */ char **line
)

{

    char w[100]; // buffer for parameter
    uservar *uvar; // pointer to user variable entry
    long long v;
    result r;

    getword(line, w); // get name of variable
    // get value for variable
    r = getparam(line, &v);
    if (r != result_ok) return r;

    // try searching the user variables list
    uvar = fndvar(w);
    if (uvar) { // found

        // place value of variable
        uvar->val = v;

    } else { // not found

        r = pushvar(w, v); // enter new variable
        if (r != result_ok) return r; // error

    }

    return result_ok; // return result ok

}

/**
 *
 * Input user variable
 *
 * Inputs either a new user variable or resets an existing one.
 *
 * \returns Standard discdiag error code.
 * 
 */

result command_input(
    /** Remaining command line */ char **line
)

{

    char linebuffer[250]; // Holder for input line
    char w[100]; // buffer for parameter
    uservar *uvar; // pointer to user variable entry
    long long v;
    result r;
    int b;

    r = result_ok; // set result ok
    getword(line, w); // get name of variable
    // get value for variable from user
    b = readline(stdin, linebuffer, sizeof(linebuffer));
    if (chkbrk() || b) {
        
        if (exiterror) r = result_exit; // exit diagnostic
        
    } else {
    
        v = strtoul(linebuffer, NULL, 0); // find number
    
        // try searching the user variables list
        uvar = fndvar(w);
        if (uvar) { // found

            // place value of variable
            uvar->val = v;

        } else { // not found

            r = pushvar(w, v); // enter new variable

        }
        
    }

    return r; // return result

}

/**
 *
 * Local variable
 *
 * Creates a new variable as a local. This is sometimes required in a procedure
 * to keep the variable from referring to an outside variable. A new variable
 * is created regardless of any existing variable of the same name.
 *
 * Right now, we don't check if the user creates any number of new variables
 * in the same procedure, or even if we are in procedure. THat result would not
 * be useful, but does no harm.
 *
 * \returns Standard discdiag error code.
 * 
 */

result command_local(
    /** Remaining command line */ char **line
)

{

    char w[100]; // buffer for parameter
    result r;

    getword(line, w); // get name of variable
    r = pushvar(w, 0); // enter as new variable
    if (r != result_ok) return r; // error

    return result_ok; // return result ok

}

/**
 *
 * Reset random number generator
 *
 * Resets the random number generator to its starting sequence, which is by
 * default a seen of 1.
 *
 * \returns Standard discdiag error code.
 * 
 */

result command_srand(
    /** Remaining command line */ char **line
)

{

    seed = 42; // reset random number generator

    return result_ok; // return result ok

}

/**
 *
 * List program store
 *
 * Lists the program store with line numbers.
 *
 * \returns Standard discdiag error code.
 * 
 */

result command_list(
    /** Remaining command line */ char **line
)

{

    linestr *p;
    int n;
    uservar *up;
    int lc;
    int bk;
    result r;

    printf("\n");
    printf("Program store:\n");
    printf("\n");
    r = result_ok; // set ok result code
    bk = 0; // clear break flag
    p = editroot; // index first line
    n = 1; // set 1st line
    linecounter = 0; // set 1st line on screen
    while (p && !bk) { // traverse and print
 
        if (chkbrk()) {
        
            if (exiterror) r = result_exit; // exit diagnostic
            bk = 1; // set break
            
        } else {
        
            if (p->label) { // there is a line label

                up = p->params; // index parameters list
                printf("%d: %s", n, p->label);
                if (up) { // there are parameters

                    printf("(");
                    while (up) {

                        printf("%s", up->varstr);
                        if (up->next) printf(" ");
                        up = up->next; // next parameter

                    }
                    printf(")");

                }
                printf(": %s\n", p->line);

            } else {

                printf("%d: %s\n", n, p->line);

            }
            p = p->next; // go next line in list
            n++; // count
            pause(); // pause on screen full
            
        }

    }

    return r; // return result ok

}

/**
 *
 * Clear program store
 *
 * Clears the program store back to free storage.
 *
 * \returns Standard discdiag error code.
 * 
 */

result command_clear(
    /** Remaining command line */ char **line
)

{

    clrpgm(); // clear out program

    return result_ok; // return result ok

}

/**
 *
 * Save program to file
 *
 * Save the current program to a file.
 *
 * \returns Standard discdiag error code.
 * 
 */

result command_save(
    /** Remaining command line */ char **line
)

{

    char fname[100];
    linestr *p;
    FILE *fp;

    getword(line, fname); // get filename
    fp = fopen(fname, "w");
    if (!fp) {

        printf("*** Error: could not create file %s\n", fname);
        return result_error; // couldn't open file    

    }

    p = editroot; // index first line
    while (p) { // traverse and print
 
        if (p->label) { // there is a line label

            fprintf(fp, "%s: %s\n", p->label, p->line);

        } else {

            fprintf(fp, "%s\n", p->line);

        }
        p = p->next; // go next line in list

    }
    fclose(fp);

    return result_ok; // return result ok

}

/**
 *
 * Load program from file
 *
 * Loads a program from a given file.
 *
 * \returns Standard discdiag error code.
 * 
 */

result command_load(
    /** Remaining command line */ char **line
)

{

    char fname[100];
    int r;

    getword(line, fname); // get filename
    r = loadfile(fname);
    if (r) {

        printf("*** Error: cannot load file\n");
        return result_error;

    }

    return result_ok; // return result ok

}

/**
 *
 * Delete program line
 *
 * Deletes a single program line by number.
 *
 * \returns Standard discdiag error code.
 * 
 */

result command_delt(
    /** Remaining command line */ char **line
)

{

    long long num;
    result r;
    linestr *p, *l;
    loopcounter* cp;

    r = getparam(line, &num); // get line number to delete
    if (r != result_ok) return r;
    p = editroot; // index first line
    l = NULL; // set no last line
    while (p && --num) { l = p; p = p->next; } // find target line
    if (p) { // must be exact line

        // gap over line
        if (l) l->next = p->next;
        else editroot = p->next;
        // remove entry
        if (p->label) free(p->label); // free label if exists
        while (p->looplist) { // free any loop counters

            cp = p->looplist; // gap from list
            p->looplist = cp->next;
            free(cp); // free

        }
        free(p->line); // free text line
        free(p); // free the text header
        
    }

    return result_ok; // return result ok

}

/**
 *
 * End command routine
 *
 * Backs out a level from the interpreter.
 *
 * \returns Standard discdiag error code.
 * 
 */

result command_end(
    /** Remaining command line */ char **line
)

{

    // check we are immediate
    if (introot && !introot->next) {

        printf("*** Error: Nothing to return to at immediate mode\n");

        return result_error;

    }
    poplvl(); // remove a level
    *line = introot->curchr; // restore at old position

    return result_ok; // return result ok

}

/**
 *
 * Goto label
 *
 * Transfers execution to the given label.
 *
 * \returns Standard discdiag error code.
 * 
 */

result command_go(
    /** Remaining command line */ char **line
)

{

    char w[100];
    linestr *p;

    getword(line, w);
    if (!strlen(w)) {

        printf("*** Error: no label specified\n");

        return result_error;

    }
    p = fndpgm(w); // find program label
    if (!p) {

        printf("*** Error: Program label %s not found\n", w);

        return result_error;

    }
    // Set as new interp location
    if (introot) {

       introot->curlin = p;
       introot->curchr = p->line;
    
    }
    *line = p->line; // start new position

    return result_restart; // return result ok, restart line

}

/**
 *
 * if conditional
 *
 * Aborts the execution of the rest of the line if the condition is not met.
 *
 * \returns Standard discdiag error code.
 * 
 */

result command_if(
    /** Remaining command line */ char **line
)

{

    result r;
    long long v;

    r = getparam(line, &v);
    if (r != result_ok) return r;
    if (!v) { // condition not met, abort line

        while (**line) (*line)++; // just dump the rest of the line

    }

    return result_ok; // return result ok

}

/**
 *
 * Test the random number generator
 *
 * Does a simple bin test of the random number generator to validate it.
 * This is an "undocumented" command only valuable to test the diagnostic.
 *
 * \returns Standard discdiag error code.
 * 
 */

result command_testrand(
    /** Remaining command line */ char **line
)

{

    int bins[100]; // give us 100 number bins
    // top number delivered by generator (all +, all > 0)
    long long top = 9223372036854775808ull;
    long num = 1000000; // number of times to test generator
    unsigned long long r; // return random number
    long i;

    // clear the bins
    for (i = 0; i < 100; i++) bins[i] = 0; // clear counts
    // test into the bins
    for (i = 0; i < num; i++) { // run tests

        r = rand64(); // find a random number
        bins[r%100]++; // count the bin

    }
    // print out the bins
    printf("Bins:\n");
    printf("\n");
    for (i = 0; i < 100; i++) {

        printf("%ld: %d\n", i, bins[i]);

    }
    printf("\n");

    return result_ok; // return result ok

}

/**
 *
 * List variables stack
 *
 * Lists out the variables stack, this is a diagnostic.
 *
 * \returns Standard discdiag error code.
 * 
 */

result command_listvariables(
    /** Remaining command line */ char **line
)

{

    printf("Contents of variables stack:\n");
    printf("\n");
    listvar();

    return result_ok; // return result ok

}

/**
 *
 * Exit diagnostic
 *
 * Exits the program.
 *
 * \returns Standard discdiag error code.
 * 
 */

result command_exit(
    /** Remaining command line */ char **line
)

{

    return result_exit; // return exit
   
}

/**
 *
 * Exit diagnostic on error
 *
 * Causes the diagnostic to exit on encountering an error. The normal mode is
 * to return interactive entry on error. However, for files that are to be run
 * entirely via batch, it may be needed to exit with the error.
 *
 * \returns Standard discdiag error code.
 * 
 */

result command_exitonerror(
    /** Remaining command line */ char **line
)

{

    exiterror = 1; // set to exit on error

    return result_ok; // return no error
   
}

/**
 *
 * Execute command
 *                       
 * Executes a command on the passed line.
 *
 * \returns Standard discdiag error code.
 *
 */
result exec(
    /** Remaining command line */ char **line
)

{

    char w[100]; // command/parameter buffer
    command *cmd; // pointer to command table
    int found; // found command
    result r; // command result value
    linestr *fp;
    uservar *pp;
    long long val;

    getword(line, w); // get command verb
    // search program commands
    found = 0; // set no command found 
    fp = fndpgm(w); // search program label
    if (fp) { // found a program command execute it

        // process parameters
        pp = fp->params; // get the parameters list
        while (pp) { // parse and load parameters

            r = getparam(line, &val); // get a parameter
            if (r != result_ok) return r; // error
            pushvar(pp->varstr, val); // push that as variable
            pp = pp->next; // next parameter

        }
        // save our current position
        if (introot) introot->curchr = *line;
        pushlvl(fp, fp->line); // start a new interp level
        *line = fp->line; // and point to that

    } else {

        // search built-in commands
        cmd = cmdtbl; // index start of command table
        while (cmd && cmd->cmd) { // traverse until end marker seen
       
            // If the command matches, execute it
            if (!strcmp(w, cmd->cmdstr)) {
       
                r = cmd->cmd(line); // execute comand 
                found = 1; // set command was found
                cmd = NULL; // flag found
       
            }
            if (cmd) cmd++; // next command entry
       
        }
        if (!found) {
       
            printf("*** Error: Command \"%s\" invalid\n", w);

            return result_error;
       
        }

    }

    return r; // return with exit status

}

/*
 * Main function
 *
 * Set up and run the diagnostic interpreter.
 *
 * \returns Non-zero on error.
 * 
 */
int main(
    /** Number of arguments */ int argc, 
    /** Argument array */      char* argv[])
{

    char *linep; // Current position in line
    int finish; // "exit" command seen flag
    result r;
    int ri;
    linestr dummyline; // We keep a dummy line for immediate mode
    int startup; // we are starting up
    linestr *fp;
    long long marktime;
    loopcounter* cp;
    double time;
    int error_result;

    printf("Disc Diagnostic 2.1\n");
    printf("\n");
    printf("Enter ? or Help for command list\n");
    printf("\n");

    //
    // Initialize the external I/O package
    // 
    initio();
    //
    // Set up ctl-c handler. We don't check if it fails, this would simply mean
    // that the old mode, break out of program, is in effect.
    //
    // Note you will want to comment this out for debugging, since it removes
    // your ability to stop the program if it hangs.
    //
    breakflag = 0; // clear any break
    signal(SIGINT, ctlchandler);
    signal(SIGABRT, ctlchandler);
    //
    // Set up other globals
    //
    currentdrive = -1; // set no drive active
    writeprot = 1; // set write protect by default
    curmode = compmode_one; // set to compare one by default
    varroot = NULL; // clear variables root
    editroot = NULL; // clear edit buffer
    introot = NULL; // clear interpreter stack
    ctlroot = NULL; // clear controls root
    finish = 0; // set not end
    error_result = 0; // set no error
    exiterror = 0; // set do not exit diagnostic on error
    // try to find and load our init file
    ri = loadfile("discdiag.ini"); // try to read it
    if (!ri) {

        printf("Init file loaded\n");
        printf("\n");

    }
    // fill out the dummy line
    dummyline.next = NULL; // no next
    dummyline.label = NULL; // no label
    dummyline.line = linebuffer; // Index standard input buffer
    dummyline.looplist = NULL; // no loop counters
    // index line buffer
    linep = linebuffer; // index command line
    startup = 1; // set startup active
    do {

        // go to next line
        nxtlin:

        linep = linebuffer; // index line
        // free any loopcounters in line
        while (dummyline.looplist != NULL) { // free any loop counters

            cp = dummyline.looplist; // gap from list
            dummyline.looplist = cp->next;
            free(cp); // free

        }
        pushlvl(&dummyline, linep); // push as new interpreter level
        rstlin(); // reset all line counters
        if (startup) { // This is the first command execute

            // see if an init command exists
            fp = fndpgm("init");
            if (fp) { // there is an init command, execute it
           
                // mark time
                marktime = gettim();
                // clear the statistics block
                iopwrite = 0.0;
                iopread = 0.0;
                bcwrite = 0.0; 
                bcread = 0.0;
                pushlvl(fp, fp->line); // start a new interp level
                linep = fp->line; // and point to that
                startup = 0; // set not in startup
                goto nxtcmd; // go commands
           
            }
            startup = 0; // set not in startup

        } else { // process time

            time = elapsed(marktime); // get the time passed in seconds
            printf("Time: %.2fs ", time);
            printscpersec("IOW: ", iopwrite, time);
            printscpersec("IOR: ", iopread, time);
            printscpersec("IO: ", iopwrite+iopread, time);
            printf("\n");
            printscpersec("BW: ", bcwrite, time);
            printscpersec("BR: ", bcread, time);
            printscpersec("BT: ", bcwrite+bcread, time);
            printf("\n");

        }
        // prompt and get command line
        printf("Diag> ");
        readline(stdin, linebuffer, sizeof(linebuffer));
        // see if we got a break during line entry
        if (chkbrk()) {

            printf("\n"); // space off unfinished line
            if (exiterror) goto exit; // exit diagnostic
            goto nxtlin; // check break

        }
        // mark time
        marktime = gettim();
        // clear the statistics block
        iopwrite = 0.0;
        iopread = 0.0;
        bcwrite = 0.0; 
        bcread = 0.0;
        while (*linep == ' ') linep++; // skip spaces
        if (isdigit(*linep)) { // leading number, is edit line

            enterline(linep); // place line in storage

        } else { // execute immediate

            do { // execute across program lines

                nxtcmd: // execute next command

                // if comment, go next line
                while (*linep == ' ') linep++; // skip spaces
                if (*linep == '!') goto nxtpgm;
                if (*linep) { // ignore blank lines
                
                    while (!finish && *linep) { // execute commands on line
                
                        // execute single command
                        r = exec(&linep);
                        // set error status for diagnostic exit purposes
                        error_result = r == result_error;
                        // dispatch special codes
                        if (r == result_exit) goto exit; // exit command
                        if (r == result_stop) goto nxtlin; // go fetch next line
                        if (r == result_error) {

                            if (exiterror) goto exit; // exit diagnostic
                            else goto nxtlin; // go fetch next line

                        }
                        if (chkbrk()) { // check break
                        
                            if (exiterror) goto exit; // exit diagnostic
                            goto nxtlin; // return to command entry
                            
                        }
                        while (*linep == ' ') linep++; // skip spaces
                        // if comment, go next line
                        if (*linep == '!') goto nxtpgm;
                        if (r != result_restart) { // check for command ending

                            if (*linep && *linep != ';') {
      
                                printf("*** Error: Invalid command termination\n");
                                goto nxtlin; // go fetch next line

                            }
                            if (*linep == ';') linep++; // skip ';'
                            while (*linep == ' ') linep++; // skip spaces

                        }
                
                    }
                
                }
                nxtpgm: // execute next program line
                if (introot && introot->next) {

                    // not in immediate mode, advance to next line in program
                    introot->curlin = introot->curlin->next;
                    // if not end of program, set start of new line
                    if (introot->curlin) linep = introot->curlin->line;
                    else {

                        // end of program, flush stack and bail
                        while (introot) poplvl();

                    }

                }

            } while (introot && introot->next); // while not immediate mode

        }
        // drain the interpreter stack
        while (introot) poplvl();

    } while (!finish); // until exit

    exit:// exit diagnostic

    // deinitialize I/O package
    deinitio();

    // exit with the last command result
    return error_result;

}
