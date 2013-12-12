/** ***************************************************************************
*
* \file
*
* \brief Stub I/O module
*
* Emulates a disc by reading and writing to and from an array. This is for the
* purpose of bring up testing, and also helps when porting to a new platform. 
* The diagnostic can be compiled and run through complete tests without any real
* real disk I/O taking place. This module is completely compatible with CLIB,
* and so should port to any ANSI C installation.
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
* gettim      - Get high resolution (64 bit) timer
*
* elapsed     - Find elapsed time
*
* initio      - Initializes this module
* 
******************************************************************************/
#include <stdio.h>
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
long long gettim(void);
double elapsed(long long t);
void initio(void);

/**
 *
 * Size of test disc array in sectors.
 *
 */
#define SIMSEC 32

/**
 *
 * String equivalences of the first 10 phy drives
 *
 */
char* phystr[10] = {

    "Drive0",
    "Drive1",
    "Drive2",
    "Drive3",
    "Drive4",
    "Drive5",
    "Drive6",
    "Drive7",
    "Drive8",
    "Drive9"

};

/**
 *
 * Simulated disc array
 *
 */

static unsigned char simulated_disc[SECSIZE*SIMSEC];

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

    // Other than setting the drive number as active, this routine does nothing.

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

    // Not implemented, just say all drives are connected
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

    unsigned char *simp;
    long long i;

    if (phydrive < 0) {

        printf("*** Error: Physical drive not set\n");
        return 1;

    }

    // read sectors from simulated array
    simp = &simulated_disc[lba*SECSIZE]; // index source in simulated disc
    for (i = 0; i < numsec*SECSIZE; i++) *buffer++ = *simp++;
    
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

    unsigned char *simp;
    long long i;

    if (phydrive < 0) {

        printf("*** Error: Physical drive not set\n");
        return 1;

    }

    // write sectors to simulated array
    simp = &simulated_disc[lba*SECSIZE]; // index source in simulated disc
    for (i = 0; i < numsec*SECSIZE; i++) *simp++ = *buffer++;

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

    // place to caller
    *size = SECSIZE*SIMSEC;

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

    // place to caller
    *size = SECSIZE*SIMSEC;

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

    // not needed

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
 * Initialize high resolution timer
 *
 * Performs any initialization tasks for the high resolution timer faciltity.
 *
 */
void initim(void)

{

    // not implemented

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

    // Not implemented
    return 0ll;

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

    // Not implemented
    return 0.0;

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

    phydrive = -1; // set no drive is active

    //
    // Initialize high resolution timer
    //
    initim();

}
 
