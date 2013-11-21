//******************************************************************************
//
// Disc Drive diagnostic
//
// Linux I/O module
//
// Contains the Linux specific I/O functions and other support functions
//
// setdrive    - Set current physical access drive (and open it).
// getdrive    - Get the current logical drive number.
// readsector  - Read one or more sectors to a buffer.
// writesector - Write one or more sectors to a buffer.
// physize     - Get the size of the physical drive in lbas.
// testsize    - Get the size of a physical drive in lbas, but takes drive as
//               parameter.
// closedrive  - Close current drive.
// getdrvstr   - Gets the string corresponding to a given logical drive.
// chkbrk      - check user break
// gettim      - Get high resolution (64 bit) timer
// elapsed     - Find elapsed time
// initio      - Initializes this module
//
//******************************************************************************

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <linux/fs.h>
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
char* phystr[10] = {

    "/dev/sda",
    "/dev/sdb",
    "/dev/sdc",
    "/dev/sdd",
    "/dev/sde",
    "/dev/sdf",
    "/dev/sdg",
    "/dev/sdh",
    "/dev/sdi",
    "/dev/sdj"

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
 * Linux handle to phy drive
 *
 */
static int phydriveh;

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
long long frequency;

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
    phydriveh = open(phystr[drive], O_RDWR, 0);

    if (phydriveh < 0)
    {

        printf("*** Error: Could not open drive: Error: %d\n", errno);

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

    int driveh;

    //open the physical disk
    driveh = open(phystr[drive], O_RDWR, 0);

    if (driveh < 0) return 1;

    //close the disk
    close(driveh);

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

    long long o;
    int r, s;

    if (phydrive < 0) {

        printf("*** Error: Physical drive not set\n");
        return 1;

    }

    //seek to the specified LBA
    o = lba * (long long)SECSIZE;
    lseek64(phydriveh, o, 0);

    // set size
    s = SECSIZE * (int)numsec;

    // read the sector
    r = read(phydriveh, buffer, s);
    if(r != s) {

        printf("*** Error: Could not read: Error: %d\n", errno);

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

    long long o;
    int r, s;

    if (phydrive < 0) {

        printf("*** Error: Physical drive not set\n");
        return 1;

    }

    //seek to the specified LBA
    o = lba * (long long)SECSIZE;
    lseek64(phydriveh, o, 0);

    // set size
    s = SECSIZE * (int)numsec;

    // write the sector
    r = write(phydriveh, buffer, s);
    if(r != s) {

        printf("*** Error: Could not write: Error: %d\n", errno);

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

    if (phydrive < 0) {

        printf("*** Error: Physical drive not set\n");
        return 1;

    }

    // find drive total size
    ioctl(phydriveh, BLKGETSIZE64, size);

    return 0;

}

/**
 *
 * Test size of physical disc
 *
 * Same as physize, but works on disks that are not opened.
 * Takes the number of the disk.
 *
 * Returns 0 on succeed, 1 on fail.
 *
 */
int testsize(
    /** drive number to set */ int drive,
    /** return size of disc */ long long *size
)

{

    int driveh;

    //open the physical disk
    driveh = open(phystr[drive], O_RDWR, 0);

    if (driveh < 0) return 1;

    // find drive total size
    ioctl(driveh, BLKGETSIZE64, size);
    
    // close drive
    close(driveh);
    
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
        close(phydriveh);

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
 * Initialize I/O package
 *
 * Sets up this package.
 *
 */
void initio(void)

{

    printf("Linux interface\n");
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
 
