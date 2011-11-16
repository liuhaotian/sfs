/* -*-C-*-
 *******************************************************************************
 *
 * File:         simpledisk.h
 * RCS:          $Id: sdisk.h,v 1.2 2009/11/10 21:17:25 npb853 Exp $
 * Description:  Simple Disk Interface - Very simple logical disk.
 *               It imulates random I/O errors.
 * Author:       Fabian E. Bustamante
 *               Northwestern Systems Research Group
 *               Department of Computer Science
 *               Northwestern University
 * Created:      Tue Nov 05, 2002 at 07:45:48
 * Modified:     Wed Nov 17, 2004 at 13:47:23 fabianb@cs.northwestern.edu
 * Language:     C
 * Package:      N/A
 * Status:       Experimental (Do Not Distribute)
 *
 * (C) Copyright 2002, Northwestern University, all rights reserved.
 *
 *******************************************************************************
 */

#ifndef SIMPLEDISK_H
#define SIMPLEDISK_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define SD_SECTORSIZE  512
#define SD_NUMSECTORS  2000

#define SD_RELIABILITY 0.95

#define SD_PERIOD 2147483647.0

/* disk errors */
typedef enum {
    E_MEM_OP, E_INVALID_PARAM, E_OPENING_FILE, E_WRITING_FILE, E_READING_FILE,
} SDError_t;

typedef struct sector {
    char data[SD_SECTORSIZE];
} Sector;

extern SDError_t sderrno; /* to see what happen with disk op */

extern int SD_initDisk();
extern int SD_finalizeDisk();
extern int SD_saveDisk(char* file);
extern int SD_loadDisk(char* file);
extern int SD_read(int sector, void *buf);
extern int SD_write(int sector, void *buf);

#endif /* !SIMPLEDISK_H */
