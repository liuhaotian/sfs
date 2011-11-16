/* -*-C-*-
 *******************************************************************************
 *
 * File:         sdisk.c
 * RCS:          $Id: sdisk.c,v 1.1 2009/11/12 09:19:51 jot836 Exp $
 * Description:  Simple logical disk
 %               It includes random read/write errors
 * Author:       Fabian E. Bustamante
 *               Northwestern Systems Research Group
 *               Department of Computer Science
 *               Northwestern University
 * Created:      Tue Nov 05, 2002 at 07:49:05
 * Modified:     Wed Nov 17, 2004 at 14:26:04 fabianb@cs.northwestern.edu
 * Language:     C
 * Package:      N/A
 * Status:       Experimental (Do Not Distribute)
 *
 * (C) Copyright 2002, Northwestern University, all rights reserved.
 *
 *******************************************************************************
 */

#include <string.h>
#include "sdisk.h"

static int threshold;

static Sector *disk; /* disk in memory - static makes it
 private to the file */
SDError_t sderrno; /* to see what happen with disk op */

static long long numReads;
static long long numWrites;
static long long numBlocksSeeked;
static long long lastAccessedBlock;

/*
 * SD_initDisk: Initialize disk area - CALL THIS FIRST
 *
 * Parameters: -
 *
 * Returns: 0 if OK, -1 otherwise
 *
 */

int SD_initDisk() {

    /* Get all the memory needed at once not to break with the virtual
     image of a disk */
    disk = (Sector*) calloc(SD_NUMSECTORS, sizeof(Sector));
    if (disk == NULL) {
        sderrno = E_MEM_OP;
        return -1;
    }
    threshold = (int) (SD_RELIABILITY * SD_PERIOD);
    numReads = 0;
    numWrites = 0;
    numBlocksSeeked = 0;
    lastAccessedBlock = 0;
    return 0;
} /* !SD_initDisk */

/*
 * SD_finalizeDisk: Clean up our virtual disk
 *
 * Parameters: -
 *
 * Returns: -
 *
 */

int SD_finalizeDisk() {
    if (disk != NULL)
        free(disk);
    fprintf(stdout, "SD: Number of reads: %20lld\tNumber of writes: %20lld\tNumber of blocks seek over: %20lld\n",
            numReads, numWrites, numBlocksSeeked);

    return 0;
} /* !SD_finalizeDisk */

/*
 * SD_saveDisk: Save current disk image to disk - careful it
 *   overwrites a pre-existing file
 * 
 * Parameters:
 *
 * Returns:
 *
 */

int SD_saveDisk(char* file) {
    FILE* diskFile;

    /* parameters check */
    if (file == NULL) {
        sderrno = E_INVALID_PARAM;
        return -1;
    }

    /* open disk file */
    if ((diskFile = fopen(file, "w")) == NULL) {
        sderrno = E_OPENING_FILE;
        return -1;
    }

    /* and write the image in */
    if ((fwrite(disk, sizeof(Sector), SD_NUMSECTORS, diskFile))
            != SD_NUMSECTORS) {
        fclose(diskFile);
        sderrno = E_WRITING_FILE;
        return -1;
    }

    /* clean up and return */
    fclose(diskFile);

    return 0;
} /* !SD_saveDisk */

/*
 * SD_loadDisk: Load current disk image from disk; the virtual disk
 *   MUST be created first
 *
 * Parameters: file with disk image
 *
 * Returns:
 *
 */
int SD_loadDisk(char* file) {
    FILE* diskFile;

    /* parameters check */
    if (file == NULL) {
        sderrno = E_INVALID_PARAM;
        return -1;
    }

    /* open the diskFile */
    if ((diskFile = fopen(file, "r")) == NULL) {
        sderrno = E_OPENING_FILE;
        return -1;
    }

    /* read disk image into memory */
    if ((fread(disk, sizeof(Sector), SD_NUMSECTORS, diskFile)) != SD_NUMSECTORS) {
        fclose(diskFile);
        sderrno = E_READING_FILE;
        return -1;
    }

    /* clean up and return */
    fclose(diskFile);
    return 0;
} /* !SD_loadDisk */

/*
 * SD_read: Try to read a secotr; randomly generates an error.
 *           Assume pre-allocated memory
 *
 * Parameters: Sector number and buffer to write it in
 *
 * Returns: 0 upon successful completion, -1 otherwise
 *
 */

int SD_read(int sector, void *buf) {
    /* parameters check */
    if ((sector < 0 || (sector >= SD_NUMSECTORS)) || (buf == NULL)) {
        sderrno = E_INVALID_PARAM;
        return -1;
    }

#ifdef SD_WITHERROR
    /* error in sector */
    if (rand() > threshold) {
        sderrno = E_READING_FILE;
        return -1;
    }
#endif	/* !SD_WITHERROR */

    /* copy the memory for the user */
    if ((memcpy((void*) buf, (void*) (disk + sector), sizeof(Sector))) == NULL) {
        sderrno = E_MEM_OP;
        return -1;
    }

    numReads++;
    numBlocksSeeked += abs(lastAccessedBlock - sector);
    lastAccessedBlock = sector;
    return 0;
} /* !SD_read */

/*
 * SD_write: Try to write a sector; randomly generates an error.
 *
 * Parameters: sector number and buffer to write from
 *
 * Returns: 0 upon successful completion, -1 otherwise
 *
 */

int SD_write(int sector, void *buf) {
    /* parameters check */
    if ((sector < 0) || (sector >= SD_NUMSECTORS) || (buf == NULL)) {
        sderrno = E_INVALID_PARAM;
        return -1;
    }

#ifdef SD_WITHERROR
    /* error in block */
    if (rand() > threshold) {
        sderrno = E_WRITING_FILE;
        return -1;
    }
#endif	/* !SD_WITHERROR */

    /* copy the memory for the user */
    if ((memcpy((void*) (disk + sector), (void*) buf, sizeof(Sector))) == NULL) {
        sderrno = E_MEM_OP;
        return -1;
    }
    numWrites++;
    numBlocksSeeked += abs(lastAccessedBlock - sector);
    lastAccessedBlock = sector;
    return 0;
} /* !SD_write */
