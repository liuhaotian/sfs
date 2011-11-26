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
	int size;
	int numsector;// how many sectors is been used
	int	status;//	0 means unused, 1 means it is a directory, 2 means it is a file
	int	toblock[7];//	to the sector ID
	int	toinode;// to next inode
} inode_t;

typedef struct {// file descriptor sturcture in memory
	int	fptab[MAXFPTAB];// the inode of the file
	int	pos[MAXFPTAB];
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
void*	inode_read(int inode);//	inode is the index of the inode array, don't forget to free it, return NULL not found!
int		inode_append(int inode);// only append a sector fot that inode, and fill the bitmap, return 0 successfully, return -1 fail
void	inode_write(int inode, void* data);//	data is the point in the memory, you should append the inode first!!!!!
void	inode_erase(int inode);//	erase the inode, including emptybitmap and init_inode

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
	mainfptab = malloc(sizeof(fptab_t));

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

	// init table
	for (i = 0; i < MAXFPTAB; ++i)
	{
		(*mainfptab).fptab[i] = 0;
		(*mainfptab).pos[i] = 0;
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
	
	strcpy((*thisdir).name, ".");
	strcpy((*upperdir).name, "..");
	(*thisdir).inode = 0;//	they point to the same inode, because root has no upper dir.
	(*upperdir).inode = 0;
	while(SD_write((*maindisk).inode[0].toblock[0], (void*)thisdir));//	write back the root as a file
	
	cwd = 0; // cwd indicate current working dir is inode[0], it is root dir
	
	
	// write back the disk_t
	for(i = 0; i < sizeof(disk_t)/SD_SECTORSIZE + 1 * (sizeof(disk_t)%SD_SECTORSIZE != 0); ++i)
	{
		while(SD_write(i, (void*)maindisk + i * SD_SECTORSIZE));
	}
	//SD_write(0, char *buf);
//	free(maindisk);// always keep it.
//	maindisk = 0;
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
	void* thisdir = inode_read(cwd);
	file_t* tmpfile = thisdir;
	
	//	find a place to save the "dir" file within the cwd
	void* tmpend = (*maindisk).inode[cwd].numsector * SD_SECTORSIZE + thisdir - sizeof(file_t);//	the last file
	while(1){
		tmpfile = (void*)tmpfile + sizeof(file_t);
		
		if((void*)tmpfile >= tmpend){
			//	there is not enough space to save it
			if(inode_append(cwd)){
				free(thisdir);
				return -1;
			}
			void* tmpdir = malloc((*maindisk).inode[cwd].numsector * SD_SECTORSIZE);// use a new memory
			tmpfile = tmpdir + ((void*)tmpfile - thisdir);
			memcpy(tmpdir, thisdir, ((*maindisk).inode[cwd].numsector - 1 )* SD_SECTORSIZE);
			free(thisdir);
			thisdir = tmpdir;
			break;
		}
		if((*tmpfile).name[0] == 0){
			break;
		}
	}
	
	strcpy((*tmpfile).name, name);
	(*tmpfile).inode = findanemptyinode();
	if((*tmpfile).inode == -1){
		free(thisdir);
		return -1;
	}
	(*maindisk).inode[(*tmpfile).inode].numsector = 1;
	(*maindisk).inode[(*tmpfile).inode].status = 1;
	(*maindisk).inode[(*tmpfile).inode].toblock[0] = findanemptysector();
	if((*maindisk).inode[(*tmpfile).inode].toblock[0] == -1){
		free(thisdir);
		return -1;
	}
	fillbitmap((*maindisk).inode[(*tmpfile).inode].toblock[0]);
	
	char data[512]="";
	
	file_t* newdir;//	"."
	file_t* upperdir;//	".."
	newdir = (void*)data;
	upperdir = (void*)data + sizeof(file_t);
	strcpy((*newdir).name, ".");
	strcpy((*upperdir).name, "..");
	(*newdir).inode = (*tmpfile).inode;//	new dir's inode
	(*upperdir).inode = cwd;
	while(SD_write((*maindisk).inode[(*tmpfile).inode].toblock[0], (void*)newdir));
	
	
	//	write back the current working dir
	inode_write(cwd, thisdir);
		
	free(thisdir);
	return 0;
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
	int prevcwd = cwd;
	if(name[0] == 0)
	{
		return 0;
	}
	else if(name[0] == '/'){
		cwd = 0;// change to root;
		if(sfs_fcd(name + 1)){//fail
			cwd = prevcwd;
			//puts("sfs_fcd: dir not found!");
			return -1;
		}
		else{
			return 0;
		}
	}
	void* thisdir = inode_read(cwd);
	file_t* tmpfile = thisdir;
	int i,slash = 0;
	for(i = 0; name[i] != 0; ++i)
	{
		if(name[i] == '/'){
			slash = i;
			//name[i] = 0;
			break;
		}
	}
	
	void* tmpend = (*maindisk).inode[cwd].numsector * SD_SECTORSIZE + thisdir - sizeof(file_t);//	the last file
	while(1){
		tmpfile = (void*)tmpfile + sizeof(file_t);
		if(((void*)tmpfile >= tmpend) || (*tmpfile).name[0] == 0){
			// 404 not found		
			break;
		}
		if(strncmp((*tmpfile).name,name, i) == 0){//	we find the dir
			if((*maindisk).inode[(*tmpfile).inode].status == 1){
				//	yes it is also a dir
				cwd = (*tmpfile).inode;
				if(slash){// we still need to fina out the dir
					if(sfs_fcd(name + slash + 1)){//fail
						cwd = prevcwd;
						//puts("sfs_fcd: dir not found!");
						return -1;
					}
					else{
						return 0;
					}
				}
				return 0;
			}			
		}		
	}
	
	//puts("sfs_fcd: dir not found!");
	return -1;
//    return -1;
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
	void* thisdir = inode_read(cwd);
	file_t* tmpfile = thisdir;
	
	//	find all the file within the cwd
	void* tmpend = (*maindisk).inode[cwd].numsector * SD_SECTORSIZE + thisdir - sizeof(file_t);//	the last file
	while(1){
		
		
		if((void*)tmpfile >= tmpend){
			break;
		}
		if((*tmpfile).name[0] == 0){
			break;
		}
		else if((*tmpfile).name[0] == '.'){
			tmpfile = (void*)tmpfile + sizeof(file_t);
			continue;
		}
		fprintf(f, "%s\n", (*tmpfile).name);
		//puts((*tmpfile).name);
		tmpfile = (void*)tmpfile + sizeof(file_t);
	}
		
	free(thisdir);
	return 0;
//	return -1;
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
    // look through cwd inode for name, store int index for file inode, if not there make new inode file, store in cwd inode, and store inode as return index
	void* currentdir = inode_read(cwd);
	file_t* tmpfile = currentdir;

	int filenode; // storing inode index
	int newfile = 0; // to continue and make newfile or not

	void* tmpend = (*maindisk).inode[cwd].numsector * SD_SECTORSIZE + currentdir - sizeof(file_t); // last file
	while (1) {		
		// look through cwd for name match
		if ((void*)tmpfile >= tmpend) { // if end and we found nothing
			newfile = 1;
			break;
		}

		if ( !strcmp( (*tmpfile).name, name )) { // found a matching file
			filenode = (*tmpfile).inode; // set the inode			
			break;
		}

		tmpfile = (void*)tmpfile + sizeof(file_t);
	}

	if (newfile) { // need to create a newfile	
		// get inode, add file_t to end of cwd, and set filenode to the inode
		tmpfile = currentdir;
		while (1) {
			tmpfile = (void*)tmpfile + sizeof(file_t);
			
			if ( (void*)tmpfile >= tmpend ) { // not enough space, need to append our inode with additional sector
				if (inode_append(cwd)) {
					free(currentdir);
					return -1;
				}
				
				void* tmp = malloc((*maindisk).inode[cwd].numsector * SD_SECTORSIZE); // new memory allocated
				tmpfile = tmp + ((void*)tmpfile - currentdir);
				
				memcpy(tmp, currentdir, ((*maindisk).inode[cwd].numsector - 1) * SD_SECTORSIZE);
				free(currentdir);
				currentdir = tmp;
				break;
			}
			if ((*tmpfile).name[0] == 0)
				break;
		}

		strcpy( (*tmpfile).name, name); // copy our name to the tmpfile
		(*tmpfile).inode = findanemptyinode();
		inode_write(cwd, currentdir); // write it back
		if ((*tmpfile).inode == -1) { // couldn't find an empty inode
			free(currentdir);
			return -1; 	
		}
		(*maindisk).inode[(*tmpfile).inode].numsector = 1; // initialize our new file's inode values
		(*maindisk).inode[(*tmpfile).inode].status = 2; // a file
		(*maindisk).inode[(*tmpfile).inode].size = 0; //size
		(*maindisk).inode[(*tmpfile).inode].toblock[0] = findanemptysector();
		
		char data[512] = "";
		while(SD_write((*maindisk).inode[(*tmpfile).inode].toblock[0], (void*)data));
		
		if ((*maindisk).inode[(*tmpfile).inode].toblock[0] == -1) { // couldn't find an empty sector for file's data
			free(currentdir);
			return -1;
		}
		fillbitmap((*maindisk).inode[(*tmpfile).inode].toblock[0]); // new sector to include in our bitmap
		
		filenode = (*tmpfile).inode; // set our new file inode to the one just created	
	}

	// look through table for first empty, set it to int file inode and return index of array
	int i = 0;
	while (i < MAXFPTAB)
	{
		if ( (*mainfptab).fptab[i] == 0) { // found an empty slot
			(*mainfptab).fptab[i] = filenode; // set it to our file inode
			free(currentdir);
			return i + 1; // the index + 1 for the file descriptor
		}
		i++;
	}
	free(currentdir);
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
    // just free the table array entry for index fileID, return 0
	int i = fileID - 1;

	if (i < 0 || i > MAXFPTAB - 1) // don't allow out of bounds array checks
			return -1;

	if ( (*mainfptab).fptab[i] != 0 ) {
		(*mainfptab).fptab[i] = 0;
		(*mainfptab).pos[i] = 0; //set our position back to zero
		return 0;
	}
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
    // grab the inode from the file table
		int i = fileID - 1;
		
		if (i < 0 || i > MAXFPTAB - 1) // don't allow out of bounds array checks
			return -1;

		int inode;
		if ( (inode = (*mainfptab).fptab[i]) == 0)
			return -1;
		
		// check paramaters for trickery
		if (((*mainfptab).pos[i] + length > (*maindisk).inode[inode].size)) // rescale the length to fit within bounds
			length = (*maindisk).inode[inode].size - (*mainfptab).pos[i];
		if (length <= 0)
			return -1;
			
		void* thisfile = inode_read(inode);
		
		memcpy(buffer, thisfile + (*mainfptab).pos[i], length); // copying file from the current read/write position into buffer by length
		
		// and set the new pos
		(*mainfptab).pos[i] += length;
		
		free(thisfile);
		
		return length;
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
		// grab the inode from the file table
		int i = fileID - 1;

		if (i < 0 || i > MAXFPTAB - 1) // don't allow out of bounds array checks
			return -1;
		
		int inode;
		if ( (inode = (*mainfptab).fptab[i]) == 0)
			return -1;
		
		// check for trickery
		if (length <= 0)
			return -1;
		
		void *thisfile = inode_read(inode); // the data stream of the file initially on the disk
		
		// 2 cases 
		// Case 1: r/w pos + length <= numsectors * SECTORSIZE
		if ((*mainfptab).pos[i] + length <= (*maindisk).inode[inode].numsector * SD_SECTORSIZE) {
			memcpy(thisfile + (*mainfptab).pos[i], buffer, length); // copy buffer to the file 
			inode_write(inode, thisfile); // write back to disk
			
			(*mainfptab).pos[i] += length;
			(*maindisk).inode[inode].size = ((*mainfptab).pos[i] > (*maindisk).inode[inode].size)? (*mainfptab).pos[i] : (*maindisk).inode[inode].size;
			free(thisfile);
			return length;
		}
		
		// Case 2: r/w pos + length > numsectors * SECTORSIZE
		else {
			int newnumsector = (((*mainfptab).pos[i] + length) / SD_SECTORSIZE ) + 1;
			while (1) {
				if ((*maindisk).inode[inode].numsector == newnumsector)
					break;
				
				if (inode_append(inode)) // append the new sector onto our inode_append, which also increases numsector
					return -1;
			}
			
			// now we should have enough space
			// malloc a new temporary file
			void *tempfile = malloc((*maindisk).inode[inode].numsector * SD_SECTORSIZE);
			
			// now copy the old memory to the new malloc'ed file
			memcpy(tempfile, thisfile, (*maindisk).inode[inode].size);
			free(thisfile);
			thisfile = tempfile;
			
			memcpy(thisfile + (*mainfptab).pos[i], buffer, length); // copy buffer to the file 
			inode_write(inode, thisfile); // write back to disk
			(*mainfptab).pos[i] += length;
			int prevsize = (*maindisk).inode[inode].size;
			(*maindisk).inode[inode].size = (*mainfptab).pos[i];
			
			free(thisfile);
			return ( (*maindisk).inode[inode].size - prevsize);
		}
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
    // grab the inode from the file table
		int i = fileID - 1;

		if (i < 0 || i > MAXFPTAB - 1) // don't allow out of bounds array checks
			return -1;
		
		int inode;
		if ( (inode = (*mainfptab).fptab[i]) == 0)
			return -1;
		
		// check paramaters for trickery
		if (position <= 0 || position >= (*maindisk).inode[inode].size)
			return -1;
		
		// and set the new pos
		(*mainfptab).pos[i] = position;
		
		return position;
} /* !sfs_lseek */

/*
 * sfs_rm: removes a file in the current directory by name if it exists.
 *
 * Parameters: file name
 *
 * Returns: 0 on success, or -1 if an error occurred
 */
int sfs_rm(char *file_name) {
	void* thisdir = inode_read(cwd);
	file_t* tmpfile = thisdir;
	
	//	find the file within the cwd
	void* tmpend = (*maindisk).inode[cwd].numsector * SD_SECTORSIZE + thisdir - sizeof(file_t);//	the last file
	while(1){
		tmpfile = (void*)tmpfile + sizeof(file_t);
		
		if(((void*)tmpfile >= tmpend) || ((*tmpfile).name[0] == 0)){
			//	404 not found
			free(thisdir);
			return -1;
		}
		if(strcmp((*tmpfile).name, file_name) == 0){
			break;
		}
	}

	//    erase the inode
	inode_erase((*tmpfile).inode);
	
	strcpy((*tmpfile).name, ".");
	(*tmpfile).inode = cwd;
	
	//	write back the current working dir
	inode_write(cwd, thisdir);
		
	free(thisdir);
	return 0;
//return -1;
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
	(*inode).numsector = 0;
	(*inode).size = 0;
	int i;
	for(i = 0; i < 7; ++i)
	{
		(*inode).toblock[i] = 0;
	}
	(*inode).toinode = -1;
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
		if(!(bitmap[ret/8] & (1<<(ret%8)))){
			return ret;
		}
	}
	
	//puts("findanemptysector: no sector available.");
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
	
	//puts("findanemptyinode: no inode available!");
	return -1;
}

void*	inode_read(int inode){
	void* ret = malloc((*maindisk).inode[inode].numsector * SD_SECTORSIZE);
	
	int tmpinode = inode;
	int i = 0;
	while(1){
		while(SD_read((*maindisk).inode[tmpinode].toblock[i%7], ret + i * SD_SECTORSIZE));
		i++;
		if(i%7 ==0){
			
			tmpinode = (*maindisk).inode[tmpinode].toinode;
		}
		if((tmpinode == -1) || ((*maindisk).inode[tmpinode].toblock[i%7] == 0)){
			break;
		}
	}
	return ret;
}

int		inode_append(int inode){
	int i = 0;
	int tmpinode = inode;
	while(1){
		i++;
		if(i%7 ==0){
			if((*maindisk).inode[tmpinode].toinode == -1){
				if(-1 == ((*maindisk).inode[tmpinode].toinode = findanemptyinode())){
					return -1;
				}
				tmpinode = (*maindisk).inode[tmpinode].toinode;
				if(-1 == ((*maindisk).inode[tmpinode].toblock[i%7] = findanemptysector())){
					return -1;
				}
				fillbitmap((*maindisk).inode[tmpinode].toblock[i%7]);
				(*maindisk).inode[inode].numsector++;
				break;				
			}
			tmpinode = (*maindisk).inode[tmpinode].toinode;
		}
		if((*maindisk).inode[tmpinode].toblock[i%7] == 0){
			if(-1 == ((*maindisk).inode[tmpinode].toblock[i%7] = findanemptysector())){
				return -1;
			}
			fillbitmap((*maindisk).inode[tmpinode].toblock[i%7]);
			(*maindisk).inode[inode].numsector++;
			break;
		}
	}
	return 0;
}

void	inode_write(int inode, void* data){
	int i = 0;
	int tmpinode = inode;
	while(1){
		while(SD_write((*maindisk).inode[tmpinode].toblock[i%7], (void*)data + i * SD_SECTORSIZE));
			
		i++;
		if(i%7 ==0){
			tmpinode = (*maindisk).inode[tmpinode].toinode;
		}
		if((tmpinode == -1) || ((*maindisk).inode[tmpinode].toblock[i%7] == 0)){//	it can't be
			break;
		}
	}
}

void	inode_erase(int inode){
	int tmpinode = inode;
	int preinode;
	int i = 0;
	while(1){
		emptybitmap((*maindisk).inode[tmpinode].toblock[i%7]);
		
		i++;
		if(i%7 ==0){
			preinode = tmpinode;
			tmpinode = (*maindisk).inode[tmpinode].toinode;
			init_inode(&((*maindisk).inode[preinode]));
		}
		if(tmpinode == -1){
			break;
		}
		if((*maindisk).inode[tmpinode].toblock[i%7] == 0){
			init_inode(&((*maindisk).inode[tmpinode]));
			break;
		}	
	}
}
