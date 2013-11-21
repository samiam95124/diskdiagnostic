//******************************************************************************
//
// Disc I/O definition file for discdiag
//
// Defines the I/O calls for discdiag.
// 
//******************************************************************************

#ifndef __DISCIO_H__
#define __DISCIO_H__

/**
 *
 * Total sectors that can reside in the buffer
 *
 */
#ifdef __LARGE__
#define NOSECS 16 // dos (can only hold this in a segment)
#else
#define NOSECS 256 // windows/linux
#endif

/**
 *
 * Size of a sector (same since the PDP-11 days, 256 * 16 bits)
 *
 */
#define SECSIZE 512

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

#endif
