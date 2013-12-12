/** ***************************************************************************
*
* \file
*
* \brief Windows I/O module
*
* Contains Windows specific I/O functions.
*
* setdrive    - Set current physical access drive (and open it).
*
* getdrive    - Get the current logical drive number.
*
* readsector  - Read one or more sectors to a buffer.
*
* writesector - Write one or more sectors to a buffer.
*
* physize     - Get the size of the physical drive in lbas.
*
* testsize    - Get the size of a physical drive in lbas, but takes drive as
*               parameter.
*
* closedrive  - Close current drive.
*
* getdrvstr   - Gets the string corresponding to a given logical drive.
*
* chkbrk      - check user break
*
* gettim      - Get high resolution (64 bit) timer
*
* elapsed     - Find elapsed time
*
* initio      - Initializes this module
* 
******************************************************************************/

#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <windows.h>
#include <winioctl.h>
#include "discio.h"

/**
 *
 * Exported functons declarations
 *
 */
int setdrive(int drive);
int getdrive(void);
int testdrive(int drive);
int readsector(unsigned char *buffer, long long lba, long long numsec);
int writesector(unsigned char *buffer, long long lba, long long numsec);
int physize(long long *size);
int testsize(int drive, long long *size);
void closedrive(void);
const char* getdrvstr(int drive);
int chkbrk(void);
long long gettim(void);
double elapsed(long long t);
void initio(void);

/**
 *
 * String equivalences of the first 10 phy drives
 *
 */
LPCSTR phystr[10] = {

    "\\\\.\\PhysicalDrive0",
    "\\\\.\\PhysicalDrive1",
    "\\\\.\\PhysicalDrive2",
    "\\\\.\\PhysicalDrive3",
    "\\\\.\\PhysicalDrive4",
    "\\\\.\\PhysicalDrive5",
    "\\\\.\\PhysicalDrive6",
    "\\\\.\\PhysicalDrive7",
    "\\\\.\\PhysicalDrive8",
    "\\\\.\\PhysicalDrive9"

};

/**
 *
 * Active drive number
 *
 * The number of the physical drive to access.
 * -1 indicates drive was never set.
 *
 */
static int phydrive;

/**
 *
 * Windows handle to phy drive
 *
 */
static HANDLE phydriveh;

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
 * Windows ticks per second for high resolution timer
 *
 */
LARGE_INTEGER frequency;

/**
 *
 * Set physical drive
 *
 * Sets the physical drive by logical number.
 * Returns 1 on error, 0 on success.
 *
 */
int setdrive(
    /** drive number to set */ int drive
)

{

    if (drive < 0) {

        printf("*** Error: Physical drive not set\n");
        return 1;

    }

    // set logical drive
    phydrive = drive;

    //open the physical disk
    phydriveh = CreateFile(phystr[drive],
                     GENERIC_READ | GENERIC_WRITE,
                     FILE_SHARE_DELETE |
                     FILE_SHARE_READ |
                     FILE_SHARE_WRITE,
                     NULL,
                     OPEN_EXISTING,
                     0,
                     NULL);

    if (phydriveh == INVALID_HANDLE_VALUE)
    {

        if (GetLastError() == 5) {

            printf("*** Error: Access was denied\n");
            printf("***        You need to run this program in a shell with administration\n");
            printf("***        level priveleges\n");

        } else {
            
            printf("*** Error: Could not open drive: Error: %d\n", GetLastError());

        }

        return 1;

    }

    return 0;

}

/**
 *
 * Get physical drive
 *
 * Gets the current drive number.
 *
 */
int getdrive(void)

{

    return phydrive; // just return

}

/**
 *
 * Test physical drive exists
 *
 * Finds if the physical drive is connected. Accepts drive numbers from 0 - 9.
 *
 * returns 0 if good, otherwise 1.
 *
 */
int testdrive(
    /** drive number to set */ int drive
)

{

    HANDLE driveh;

    //open the physical disk
    driveh = CreateFile(phystr[drive],
                     GENERIC_READ | GENERIC_WRITE,
                     FILE_SHARE_DELETE |
                     FILE_SHARE_READ |
                     FILE_SHARE_WRITE,
                     NULL,
                     OPEN_EXISTING,
                     0,
                     NULL);

    if (driveh == INVALID_HANDLE_VALUE) return 1;

    //close the disk
    CloseHandle(driveh);

    return 0;

}

/**
 *
 * Read sector to buffer
 *
 * Reads the given number of sectors to the indicated buffer.
 * Returns 1 on error, 0 on success.
 *
 */
int readsector(
    /** Buffer to read sector to */       unsigned char *buffer, 
    /** Logical block address to start */ long long lba, 
    /** Number of sectors to read */      long long numsec
)

{

    long long LongOffset = 0;
    DWORD     retsize, TmpOffsetHigh, TmpOffsetLow;

    if (phydrive < 0) {

        printf("*** Error: Physical drive not set\n");
        return 1;

    }

    //seek to the specified LBA
    LongOffset = lba * (long long)SECSIZE;
    TmpOffsetLow = (DWORD)LongOffset;
    TmpOffsetHigh = (DWORD)(LongOffset >> 32);
    SetFilePointer(phydriveh, TmpOffsetLow, (PLONG)&TmpOffsetHigh, FILE_BEGIN);

    //read the sector
    BOOL result = ReadFile(phydriveh, buffer, (SECSIZE * (int)numsec), &retsize, NULL);
    if(!result)
    {

        printf("*** Error: Could not read: Error: %d\n", GetLastError());

        return 1;

    }
    
    return 0; // return good

}

/**
 *
 * Write sector from buffer
 *
 * Writes the given number of sectors to the indicated buffer.
 * Returns 1 on error, 0 on success.
 *
 */
int writesector(
    /** Buffer to write sector from */    unsigned char *buffer, 
    /** Logical block address to start */ long long lba, 
    /** Number of sectors to write */     long long numsec
)

{

    long long LongOffset = 0;
    DWORD     retsize, TmpOffsetHigh, TmpOffsetLow;

    if (phydrive < 0) {

        printf("*** Error: Physical drive not set\n");
        return 1;

    }

    //seek to the specified LBA
    LongOffset = lba * (long long)SECSIZE;
    TmpOffsetLow = (DWORD)LongOffset;
    TmpOffsetHigh = (DWORD)(LongOffset >> 32);
    SetFilePointer(phydriveh, TmpOffsetLow, (PLONG)&TmpOffsetHigh, FILE_BEGIN);

    //read the sector
    BOOL result = WriteFile(phydriveh, buffer, (SECSIZE * (int)numsec), &retsize, NULL);
    if(!result)
    {

        printf("*** Error: Could not write: Error: %d\n", GetLastError());

        return 1;

    }
    
    return 0; // return good

}

/**
 *
 * Find size of physical disc
 *
 * Returns 0 on succeed, 1 on fail.
 *
 */
int physize(
    /** return size of disc */ long long *size
)

{

    GET_LENGTH_INFORMATION li;
    BOOL r;
    DWORD rsize;

    if (phydrive < 0) {

        printf("*** Error: Physical drive not set\n");
        return 1;

    }

    // get size of disk
    r = DeviceIoControl(phydriveh, IOCTL_DISK_GET_LENGTH_INFO, NULL, 0, &li, sizeof(li), 
                        &rsize, NULL);
    if (r == 0) {

        printf("*** Error: Cannot get size of disk\n");
        return 1;

    }

    // place to caller
//    *size = ((long long) li.Length.HighPart << 32)+li.Length.LowPart;
    *size = li.Length.QuadPart;

    return 0;

}

/**
 *
 * Test size of physical disc
 *
 * Same as physize, but works on disks that are not opened.
 * Takes the number of the disk.
 *
 * Note: does not print on error.
 *
 * Returns 0 on succeed, 1 on fail.
 *
 */
int testsize(
    /** drive number to set */ int drive,
    /** return size of disc */ long long *size
)

{

    GET_LENGTH_INFORMATION li;
    BOOL r;
    DWORD rsize;

    HANDLE driveh;

//printf("testsize: begin\n");
    //open the physical disk
    driveh = CreateFile(phystr[drive],
                     GENERIC_READ | GENERIC_WRITE,
                     FILE_SHARE_DELETE |
                     FILE_SHARE_READ |
                     FILE_SHARE_WRITE,
                     NULL,
                     OPEN_EXISTING,
                     0,
                     NULL);

//printf("testsize: 1\n");
    if (driveh == INVALID_HANDLE_VALUE) return 1;

    // get size of disk
    r = DeviceIoControl(driveh, IOCTL_DISK_GET_LENGTH_INFO, NULL, 0, &li, sizeof(li), 
                        &rsize, NULL);
//printf("testsize: 2\n");
    if (r == 0) {

//printf("testsize: 3\n");
        return 1; // just return error

    }

    // place to caller
    *size = li.Length.QuadPart;

    //close the disk
    CloseHandle(driveh);
//printf("testsize: end\n");

    return 0;

}

/**
 *
 * Close out physical disk
 *
 * Closes the physical disk prior to exiting the diagnostic.
 *
 */
void closedrive(void)

{

    if (phydrive >= 0) { // disk is valid

        //close the disk
        CloseHandle(phydriveh);

    }

}

/**
 *
 * Get logical drive string
 *
 * Gets the name of the drive corresponding to the logical number.
 * Returns this as a string, or null pointer if no such drive exists.
 *
 */
const char* getdrvstr(int drive)

{

    const char* p;

    p = 0; // set no drive string

    // check drive is valid and set corresponding string
    if (drive >= 0 || drive <= 9) p = phystr[drive];

    return p;

}

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
 * Finds the elapsed time in second. Returns as a floating point value so that
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
 * Initialize I/O package
 *
 * Sets up this package.
 *
 */
void initio(void)

{

    BOOL br;

    printf("Windows interface\n");
    printf("\n");

    phydrive = -1; // set no drive is active

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

}
 
