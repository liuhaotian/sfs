/* -*-C-*-
 *******************************************************************************
 *
 * File:         sfs.c
 * RCS:          $Id: sfs.c,v 1.2 2009/11/10 21:17:25 npb853 Exp $
 * Description:  Simple File System
 * Author:       Fabian E. Bustamante
 *               Northwestern Systems Research Group
 *               Department of Computer Science
 *               Northwestern University
 * Created:      Tue Nov 05, 2002 at 07:40:42
 * Modified:     Fri Nov 19, 2004 at 15:45:57 fabianb@cs.northwestern.edu
 * Language:     C
 * Package:      N/A
 * Status:       Experimental (Do Not Distribute)
 *
 * (C) Copyright 2003, Northwestern University, all rights reserved.
 *
 *******************************************************************************
 */

#include "sfs.h"
#include "sdisk.h"
#include <string.h>

/*
 *	global variables
 */

#define MAXINODE	2000//	minimun should be SD_NUMSECTORS/7, but the wores case is: each file takes one iNode, so the total number will reach 2000
#define MAXFPTAB	2000//	for file descriptor table, it is in memory

typedef struct {//	i-node structure
	//	some attributes
	int numsector;// how many sectors is been used
	int	status;//	0 means unused, 1 means it is a directory, 2 means it is a file
	int	toblock[7];//	to the sector ID
	int	toinode;// to next inode
} inode_t;

typedef struct {// file descriptor sturcture in memory
	void*	fptab[MAXFPTAB];
} fptab_t;

typedef struct {// file sturcture for file header, it is a file sturcture in the sector
	char	name[17];// support for up to 16 characteristics, the last one should be \0
	int		inode;// point back to its inode. whether it is a ture file ot a directory is defined in inode. It is a inodeID, from 0 to 2000
} file_t;

typedef struct {// disk sturcture for disk header
	
	inode_t			inode[MAXINODE];
	unsigned char	bitmap[SD_NUMSECTORS/8];//	SD_NUMSECTORS/8, cause SD_NUMSECTORS is not power of two , we may waste the last several sectors
	// we should alloc inode[0] for root
	//inode			root;// reserve for root
} disk_t;

disk_t*		maindisk;
fptab_t*	mainfptab;
int			cwd;// current working dir, it is the inode index.

void	fillbitmap(int sector);
void	emptybitmap(int sector);
void	init_inode(inode_t* inode);
void	init_dir(inode_t* thisdirinode, inode_t* upperdirinode);
int		findanemptysector();
int		findanemptyinode();

/*
 * sfs_mkfs: use to build your filesystem
 *
 * Parameters: -
 *
 * Returns: 0 on success, or -1 if an error occurred
 *
 */
int sfs_mkfs() {
	maindisk = malloc(sizeof(disk_t));
	
	int i;
	for(i = 0; i < MAXINODE; ++i)
	{
		init_inode(&((*maindisk).inode[i]));
	}
	for(i = 0; i < SD_NUMSECTORS/8; ++i)
	{
		(*maindisk).bitmap[i] = 0;
	}
	
	//	here we plus one, because sizeof(disk_t)/SD_SECTORSIZE will be rounded, we should take consideration of the remainder
	for(i = 0; i < sizeof(disk_t)/SD_SECTORSIZE + 1 * (sizeof(disk_t)%SD_SECTORSIZE != 0); ++i)
	{
		fillbitmap(i);
	}
	
	//	init root dir
	(*maindisk).inode[0].numsector = 1;
	(*maindisk).inode[0].status = 1;
	(*maindisk).inode[0].toblock[0] = sizeof(disk_t)/SD_SECTORSIZE + 1 * (sizeof(disk_t)%SD_SECTORSIZE != 0);//	next available sector
	fillbitmap((*maindisk).inode[0].toblock[0]);
	
	file_t* thisdir;//	"."
	file_t* upperdir;//	".."
	thisdir = (void*)maindisk + (*maindisk).inode[0].toblock[0] * SD_SECTORSIZE;
	upperdir = (void*)maindisk + (*maindisk).inode[0].toblock[0] * SD_SECTORSIZE + sizeof(file_t);
	
	//	file test for George code begin
	file_t* atest;//test.txt
	atest = (void*)maindisk + (*maindisk).inode[0].toblock[0] * SD_SECTORSIZE + 2 * sizeof(file_t);
	
	strcpy((*atest).name, "test.txt");
	(*atest).inode = 1;// point to a new inode
	
	(*maindisk).inode[1].numsector = 1;
	(*maindisk).inode[1].status = 2;
	(*maindisk).inode[1].toblock[0] = sizeof(disk_t)/SD_SECTORSIZE + 1 * (sizeof(disk_t)%SD_SECTORSIZE != 0) +1;//	next available sector
	fillbitmap((*maindisk).inode[1].toblock[0]);
	
	SD_write((*maindisk).inode[1].toblock[0], "This is a test file. The file name is test.txt. There should be a EOF sign, but I haven't implemented it yet.");
	
	//	file test code end
	
	strcpy((*thisdir).name, ".");
	strcpy((*upperdir).name, "..");
	(*thisdir).inode = 0;//	they point to the same inode, because root has no upper dir.
	(*upperdir).inode = 0;
	SD_write((*maindisk).inode[0].toblock[0], (void*)thisdir);//	write back the root as a file
	
	cwd = 0; // cwd indicate current working dir is inode[0], it is root dir
	
	
	// write back the disk_t
	for(i = 0; i < sizeof(disk_t)/SD_SECTORSIZE + 1 * (sizeof(disk_t)%SD_SECTORSIZE != 0); ++i)
	{
		SD_write(i, (void*)maindisk + i * SD_SECTORSIZE);
	}
	//SD_write(0, char *buf);
	free(maindisk);
	//return -1;
	return 0;
} /* !sfs_mkfs */

/*
 * sfs_mkdir: attempts to create the name directory
 *
 * Parameters: directory name
 *
 * Returns: 0 on success, or -1 if an error occurred
 *
 */
int sfs_mkdir(char *name) {
	void* thisdir = malloc((*maindisk).inode[cwd].numsector * SD_SECTORSIZE);
	int i=0;
	int tmpinode;
	tmpinode = cwd;
	while(1){
		SD_read((*maindisk).inode[tmpinode].toblock[i%7], thisdir + i * SD_SECTORSIZE);		
		i++;
		if(i%7 ==0){
			tmpinode = (*maindisk).inode[tmpinode].toinode;
		}
		if((tmpinode == 0) || ((*maindisk).inode[tmpinode].toblock[i%7] == 0)){
			break;
		}
	}// dir read complete
	
	//	find a empty inode
	//	find a empty sector
	// if the cwd has not enough space, append a sector , append the inode.
	//	add a new dir type file into the cwd, link the inode, link the sector, numsector++
	
//	return -1;
} /* !sfs_mkdir */

/*
 * sfs_fcd: attempts to change current directory to named directory
 *
 * Parameters: new directory name
 *
 * Returns: 0 on success, or -1 if an error occurred
 *
 */
int sfs_fcd(char* name) {
    // TODO: Implement
    return -1;
} /* !sfs_fcd */

/*
 * sfs_ls: output the information of all existing files in 
 *   current directory
 *
 * Parameters: -
 *
 * Returns: 0 on success, or -1 if an error occurred
 *
 */
int sfs_ls(FILE* f) {
    // TODO: Implement
    return -1;
} /* !sfs_ls */

/*
 * sfs_fopen: convert a pathname into a file descriptor. When the call
 *   is successful, the file descriptor returned will be the lowest file
 *   descriptor not currently open for the process. If the file does not
 *   exist it will be created.
 *
 * Parameters: file name
 *
 * Returns:  return the new file descriptor, or -1 if an error occurred
 *
 */
int sfs_fopen(char* name) {
    // TODO: Implement
    return -1;
} /* !sfs_fopen */

/*
 * sfs_fclose: close closes a file descriptor, so that it no longer
 *   refers to any file and may be reused.
 *
 * Parameters: -
 *
 * Returns: 0 on success, or -1 if an error occurred
 *
 */
int sfs_fclose(int fileID) {
    // TODO: Implement
    return -1;
} /* !sfs_fclose */

/*
 * sfs_fread: attempts to read up to length bytes from file
 *   descriptor fileID into the buffer starting at buffer
 *
 * Parameters: file descriptor, buffer to read and its lenght
 *
 * Returns: on success, the number of bytes read are returned. On
 *   error, -1 is returned
 *
 */
int sfs_fread(int fileID, char *buffer, int length) {
    // TODO: Implement
    return -1;
}

/*
 * sfs_fwrite: writes up to length bytes to the file referenced by
 *   fileID from the buffer starting at buffer
 *
 * Parameters: file descriptor, buffer to write and its lenght
 *
 * Returns: on success, the number of bytes written are returned. On
 *   error, -1 is returned
 *
 */
int sfs_fwrite(int fileID, char *buffer, int length) {
    // TODO: Implement
    return -1;
} /* !sfs_fwrite */

/*
 * sfs_lseek: reposition the offset of the file descriptor 
 *   fileID to position
 *
 * Parameters: file descriptor and new position
 *
 * Returns: Upon successful completion, lseek returns the resulting
 *   offset location, otherwise the value -1 is returned
 *
 */
int sfs_lseek(int fileID, int position) {
    // TODO: Implement
    return -1;
} /* !sfs_lseek */

/*
 * sfs_rm: removes a file in the current directory by name if it exists.
 *
 * Parameters: file name
 *
 * Returns: 0 on success, or -1 if an error occurred
 */
int sfs_rm(char *file_name) {
    // TODO: Implement for extra credit
    return -1;
} /* !sfs_rm */

void fillbitmap(int sector){
	unsigned char* bitmap=(*maindisk).bitmap;
	bitmap[sector/8] |= (1<<(sector%8));
}

void emptybitmap(int sector){
	unsigned char* bitmap=(*maindisk).bitmap;
	bitmap[sector/8] &= (~(1<<(sector%8)));
}

void init_inode(inode_t* inode){
	(*inode).status = 0;
	int i;
	for(i = 0; i < 7; ++i)
	{
		(*inode).toblock[i] = 0;
	}
	(*inode).toinode = 0;
}

void init_dir(inode_t* thisdirinode, inode_t* upperdirinode){// to be done
	file_t* thisdir;
	file_t* upperdir;
	
	//	this doesn't work
	thisdir = (void*)maindisk + (*thisdirinode).toblock[0] * SD_SECTORSIZE;
	upperdir = (void*)maindisk + (*thisdirinode).toblock[0] * SD_SECTORSIZE + sizeof(file_t);
	
}

int findanemptysector(){
	int ret;
	unsigned char* bitmap=(*maindisk).bitmap;
	for(ret = 1; ret < SD_NUMSECTORS; ++ret)
	{
		if(bitmap[ret/8] & (1<<(sector%8))){
			return ret;
		}
	}
	
	printf("There is not enough room: no sector available.\n");
	return -1;
}

int findanemptyinode(){
	int ret;
	for(ret = 0; ret < MAXINODE; ++ret)
	{
		if((*maindisk).inode[ret].toblock[0] == 0){
			return ret;
		}
	}
	
	printf("There is not enough room: no inode available.\n");
	return -1;
}