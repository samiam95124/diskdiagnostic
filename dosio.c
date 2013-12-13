/** ***************************************************************************
*
* \file
*
* \brief DOS I/O module
*
* Contains DOS specific I/O functions.
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
* initio      - Initializes this module
*
* deinitio    - Deinitializes this module.
* 
******************************************************************************/

#include <stdio.h>
#include "discio.h"

/*
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
const char* getdrvstr(int drive);
void initio(void);
void deinitio(void);

/*
 *
 * Internal functions declarations
 *
 */
static void closedrive(void);

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
 * Active drive number
 *
 * The number of the physical drive to access.
 * -1 indicates drive was never set.
 *
 */
static int phydrive;

/**
 *
 * Current drive geometry information
 *
 * Stores the number of sides, tracks and sectors for the current drive.
 *
 */
static int maxsector;
static int maxtrack;
static int maxside;

/**
 *
 * Find drive parameters
 *
 * Finds the basic disk drive parameters from BIOS. Used both to get the drive
 * size and also to probe drive online status.
 *
 */

static unsigned char probestatus(
    /** Drive to check */  unsigned char drive,
    /** Returns tracks */  int *tracks,
    /** Returns sectors */ int *sectors,
    /** Returns sides */   int *sides
)

{

    unsigned char rs;
    unsigned char trackss;
    unsigned char sectorss;
    unsigned char sidess;

    // Get parameters from drive via bios
    _asm
	{

        mov ax,0000h
        mov es,ax
        mov di,ax
		mov ax,0800h
        mov dl,drive
        or  dl,80h
		int 13h
        mov rs,ah
        mov trackss,ch
        mov sectorss,cl
        mov sidess,dh

	}

    // printf("Drive status: %.2x\n", rs);
    // printf("Tracks:       %.2x\n", trackss);
    // printf("Sectors:      %.2x\n", sectorss);
    // printf("Sides:        %.2x\n", sidess);

    // place return parameters
    *tracks = trackss | (((int) sectorss & 0xc0) << 2); // combine cylinder bits
    *sectors = sectorss & 0x3f; // remove high cylinder bits
    *sides = sidess+1;

    // printf("Drive status: %.2x\n", rs);
    // printf("Tracks:       %d\n", *tracks);
    // printf("Sectors:      %d\n", *sectors);
    // printf("Sides:        %d\n", *sides);

    // exit with status
    return rs;
   
}

/**
 *
 * Set physical drive
 *
 * Sets the physical drive by logical number.
 * Returns 1 on error, 0 on success.
 *
 */
int setdrive(
    /** Drive number to set */ int drive
)

{

    unsigned char rs;
    int tracks;
    int sectors;
    int sides;

    if (drive < 0) {

        printf("*** Error: Physical drive not set\n");

        return 1;

    }
    
    closedrive(); // close any active drive

    // find status via bios
    rs = probestatus(drive, &tracks, &sectors, &sides);

    // set logical drive if no error
    if (!rs) {

        phydrive = drive;
        maxsector = sectors;
        maxtrack = tracks;
        maxside = sides;

    }

    // return sucess if no error
    return !!rs;

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

    unsigned char rs;
    int tracks;
    int sectors;
    int sides;

    // find status via bios
    rs = probestatus(drive, &tracks, &sectors, &sides);

    // return fail on any error
    return !!rs;

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

    unsigned char rs;
    unsigned char track;
    unsigned char sector;
    unsigned char side;
    unsigned char ns;
    unsigned char dr;

    if (phydrive < 0) {

        printf("*** Error: Physical drive not set\n");
        return 1;

    }

    // devolve the lba to head/track/sector, in that order
    sector = (lba % maxsector)+1; // get sector (adjusted 1-n)
    lba = lba / maxsector;
    side = lba % maxside; // get side
    lba = lba / maxside;
    track = lba % maxtrack; // get track
    sector = sector | (track & 0x300 >> 2); // place lower track bits in sector
    track &= 0xff; // and mask off bits from track

    ns = numsec; // get number of sectors as byte
    dr = phydrive; // get drive
    _asm
	{

        mov ah,02h
        mov al,ns
        les bx,buffer
        mov ch,track
        mov cl,sector
        mov dh,side
        mov dl,dr
        or  dl,80h
		int 13h
        mov rs,ah

	}

    // return fail on any error
    return !!rs;

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

    unsigned char rs;
    unsigned char track;
    unsigned char sector;
    unsigned char side;
    unsigned char ns;
    unsigned char dr;

    if (phydrive < 0) {

        printf("*** Error: Physical drive not set\n");
        return 1;

    }

    // devolve the lba to head/track/sector, in that order
    sector = (lba % maxsector)+1; // get sector (adjusted 1-n)
    lba = lba / maxsector;
    side = lba % maxside; // get side
    lba = lba / maxside;
    track = lba % maxtrack; // get track
    sector = sector | (track & 0x300 >> 2); // place lower track bits in sector
    track &= 0xff; // and mask off bits from track
    ns = numsec; // get number of sectors as byte
    dr = phydrive; // get drive
    _asm
	{

        mov ah,03h
        mov al,ns
        les bx,buffer
        mov ch,track
        mov cl,sector
        mov dh,side
        mov dl,dr
        or  dl,80h
		int 13h
        mov rs,ah

	}

    // return fail on any error
    return !!rs;

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

    unsigned char rs;
    unsigned char tracks;
    unsigned char sectors;
    unsigned char sides;

    if (phydrive < 0) {

        printf("*** Error: Physical drive not set\n");
        return 1;

    }

    // place size to caller
    *size = (long long)maxtrack*maxsector*maxside*SECSIZE;

    // return fail on any error
    return !!rs;

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

    unsigned char rs;
    int tracks;
    int sectors;
    int sides;

    // Get parameters from drive via bios
    rs = probestatus(drive, &tracks, &sectors, &sides);

    // place to caller
    *size = (long long)tracks*sectors*sides*SECSIZE;

    // return fail on any error
    return !!rs;

}

/**
 *
 * Close out physical disk
 *
 * Closes the physical disk prior to exiting the diagnostic.
 *
 */
static void closedrive(void)

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
 * Initialize I/O package
 *
 * Sets up this package.
 *
 */
void initio(void)

{

    printf("DOS/BIOS interface\n");
    printf("\n");

    phydrive = -1; // set no drive is active

}

/**
 *
 * Deinitialize I/O package
 *
 * Tears down this package.
 *
 */
void deinitio(void)

{

    closedrive(); // close any active drive

} 
 
