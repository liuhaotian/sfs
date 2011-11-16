/* -*-C-*-
 *******************************************************************************
 *
 * File:         testsd.c
 * RCS:          $Id: testsd.c,v 1.1 2009/11/09 02:46:33 npb853 Exp $
 * Description:  Driver for our Simple Disk
 * Author:       Fabian E. Bustamante
 *               Northwestern Systems Research Group
 *               Department of Computer Science
 *               Northwestern University
 * Created:      Thu Nov 07, 2002 at 15:10:17
 * Modified:     Fri Nov 19, 2004 at 16:05:29 fabianb@cs.northwestern.edu
 * Language:     C
 * Package:      N/A
 * Status:       Experimental (Do Not Distribute)
 *
 * (C) Copyright 2002, Northwestern University, all rights reserved.
 *
 *******************************************************************************
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "sdisk.h"

#define NUMSEC 10

char* program_name;

/* 
 * usage: report usage to given stream and exit
 *
 * Parameters: Where to report usage and our exit status
 *
 * Returns: -
 *
 */

void usage(FILE* stream, int status) {
    fprintf(stream, "Usage: %s -h -v -r -f FILE\n"
        "Test driver for a simple virtual disk.\n"
        "   -v \tverbose output\n"
        "   -h \tthis help message\n"
        "   -f FILE \tdisk image file (this is left behind)\n", program_name);
    exit(status);
} /* !usage */

int main(int argc, char* argv[]) {
    int i, c;
    int verbose = 0; /* verbose mode flag */
    char tmpbuf[SD_SECTORSIZE];
    char *diskFName = NULL; /* image disk file */

    program_name = argv[0];

    while ((c = getopt(argc, argv, "vhrf:")) != EOF) {
        switch (c) {
        case 'h':
            usage(stdout, 0);
            break;
        case 'v':
            ++verbose;
            break;
        case 'f':
            diskFName = strdup(optarg);
            if (verbose)
                fprintf(stdout, "Disk image file: %s\n", diskFName);
            break;
        default:
            usage(stdout, 1);
            break;
        }
    }

    if (verbose)
        fprintf(stdout, "Initialize disk before using it\n");
    if (SD_initDisk())
        fprintf(stderr, "Problems initializing disk %d\n", sderrno);

    if (verbose)
        fprintf(stdout, "Write something to disk (program name)\n");
    for (i = 0; i < NUMSEC; ++i) {
        if (SD_write(i, (void*) program_name))
            fprintf(stdout, "Error %d while writing block %d\n", sderrno, i);
        else if (verbose)
            fprintf(stdout, "Block written (%d) \n", i);
    }
    fflush(stdout);

    if (verbose)
        fprintf(stdout, "Read it back now\n");
    for (i = 0; i < NUMSEC; ++i) {
        if (SD_read(i, (void*) tmpbuf))
            fprintf(stdout, "Error %d while reading block %d\n", sderrno, i);
        else if (verbose)
            fprintf(stdout, "Block %d content %s\n", i, tmpbuf);
    }
    fflush(stdout);

    if (diskFName) { /* Need the file to check this */
        if (SD_saveDisk(diskFName))
            fprintf(stdout, "Error %d while saving disk image to %s\n",
                    sderrno, diskFName);
        else if (verbose)
            fprintf(stdout, "Disk image saved to %s\n", diskFName);

        SD_finalizeDisk(); /* clean it up before re-loading to be sure */

        if (SD_loadDisk(diskFName))
            fprintf(stdout, "Error %d while reading disk image from %s\n",
                    sderrno, diskFName);
        else if (verbose)
            fprintf(stdout, "Disk image restored from %s\n", diskFName);

        /* Do we still have the same? */
        if (verbose)
            fprintf(stdout, "Read it back now\n");
        for (i = 0; i < NUMSEC; ++i) {
            if (SD_read(i, (void*) tmpbuf))
                fprintf(stdout, "Error %d while reading block %d\n", sderrno, i);
            else if (verbose)
                fprintf(stdout, "Block %d content %s\n", i, tmpbuf);
        }
        fflush(stdout);
    }

    if (verbose)
        fprintf(stdout, "Always clean up after yourself\n");
    if (!diskFName)
        free(diskFName);
    SD_finalizeDisk();

    exit(0);
} /* !main */

