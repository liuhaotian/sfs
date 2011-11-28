/* -*-C-*-
 *******************************************************************************
 *
 * File:         testfs.c
 * RCS:          $Id: testfs.c,v 1.3 2009/11/12 09:19:51 jot836 Exp $
 * Description:  Test of file system capabilities
 * Author:       David Choffnes
 *               Northwestern Systems Research Group
 *               Department of Computer Science
 *               Northwestern University
 * Created:      Thu Nov 07, 2002 at 15:10:17
 * Modified:     Fri Nov 18, 2005 at 16:05:29 drchoffnes@cs.northwestern.edu
 * Language:     C
 * Package:      N/A
 * Status:       Experimental (Do Not Distribute)
 *
 * (C) Copyright 2005, Northwestern University, all rights reserved.
 *
 *******************************************************************************
 */

#define COMPETITION 0

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "sdisk.h"
#include "sfs.h"
#include "config.h"

//char* program_name;
bool gbIsVerbose = 0; // verbose mode flag
char* gsDiskFName = NULL; // the name of the diskfile
FILE* f_ls;


/* prototypes */
void usage(char* program_name, FILE* stream, int status);

// Tests Core
int runTests();
int initDiskTest();
int initFSTest();
int customTest();
int createSimpleFileTest();
int createSimpleFileWriteReadTest();
int singleBigFileTest();
int multipleFilesTest();
int createFolderTest();
int multipleFoldersTest();
int appendFileTest();
int multipleOpenFilesTest();
int nestedFoldersTest();
int errorTest();
int removeTest();
int perfTest();

// Tests helpers
int initDisk();
int saveAndCloseDisk();
int initAndLoadDisk();
int initFS();
int createSmallFile(char *fileName, char *data, int fsize);
int verifyFile(char *fileName, char *data, int fsize);
int singleBigFile(char *fileName, int fsize);
int multipleFiles(char *fileBaseName, int numFiles, int maxFileSize);
int createFolder(char *dirName);
int createFolders(char *dirBaseName, int numDirs);
int appendFile();
int nestedFolders(char *dirBaseName, int num);

int refreshDisk();
void initBuffer(char *buf, int fsize);
int checkBuffers(char *buf, char *cpy, int fsize, int offset);
int testFile(char* name, int fsize);

/*
 * usage: report usage to given stream and exit
 *
 * Parameters: Where to report usage and our exit status
 *
 * Returns: -
 *
 */
void usage(char *program_name, FILE* stream, int status) {
    fprintf(stream, "Usage: %s -h -v -f FILE\n"
        "Test driver for a simple virtual disk.\n"
        "   -v \tverbose output\n"
        "   -h \tthis help message\n"
        "   -f FILE \tdisk image file (this is left behind)\n", program_name);
    exit(status);
} /* !usage */

int main(int argc, char* argv[]) {
    char c;
    char* program_name = argv[0];

    while ((c = getopt(argc, argv, "vhf:")) != EOF) {
        switch (c) {
        case 'h':
            usage(program_name, stdout, 0);
            break;
        case 'v':
            gbIsVerbose = TRUE;
            break;
        case 'f':
            gsDiskFName = strdup(optarg);
            LOG(stdout, "Disk image file: %s\n", gsDiskFName);
            break;
        default:
            usage(program_name, stdout, 1);
            break;
        }
    }

    if (gsDiskFName == NULL) {
        fprintf(stdout, "You must specify a file name for this test!\n");
        usage(program_name, stdout, 1);
    }

    runTests();

    SAFE_FREE(gsDiskFName);
    return 0;
} /* !main */

/**
 * Runs the test suite.
 */
int runTests() {
    int hr = SUCCESS;

#ifndef EXTRA_CREDIT
#ifndef COMPETITION_TEST
    FILE *f_ls_base;
#else
    FILE *f_ls_compTest;
#endif
#endif

#ifdef EXTRA_CREDIT
    FILE *f_ls_extra_credit;
#endif

#if COMPETITION
    FILE *f_ls_competition;
    f_ls_competition = fopen("competition.ls", "w");
#endif

    RUN_TEST(initDiskTest());

#ifndef EXTRA_CREDIT
#ifndef COMPETITION_TEST
    f_ls_base = fopen("base.ls", "w");
    f_ls = f_ls_base;
    RUN_TEST(initFSTest());
    RUN_TEST(createSimpleFileTest());
    RUN_TEST(createSimpleFileTest());
    RUN_TEST(createSimpleFileWriteReadTest());
    RUN_TEST(singleBigFileTest());
    RUN_TEST(multipleFilesTest());
    RUN_TEST(createFolderTest());
    RUN_TEST(multipleFoldersTest());
    RUN_TEST(appendFileTest());
    RUN_TEST(multipleOpenFilesTest());
    RUN_TEST(nestedFoldersTest());
    RUN_TEST(errorTest());
#else
    f_ls_compTest = fopen("compTest.ls", "w");
    f_ls = f_ls_compTest;
    RUN_TEST(customTest());
#endif
#endif

#ifdef EXTRA_CREDIT
    f_ls_extra_credit = fopen("extra_credit.ls", "w");
    f_ls = f_ls_extra_credit;
    RUN_TEST(removeTest());
#endif

    // NOTE: We're not using this competition code this year.
#if COMPETITION
    f_ls = f_ls_competition;
    RUN_TEST(perfTest());
#endif


    Fail:

#ifndef EXTRA_CREDIT
#ifndef COMPETITION_TEST
    fclose(f_ls_base);
#else
    fclose(f_ls_compTest);
#endif
#endif

#ifdef EXTRA_CREDIT
    fclose(f_ls_extra_credit);
#endif

#if COMPETITION
    fclose(f_ls_competition);
#endif

    return hr;
}

/**
 * Testing the initialization code of the fs. Should work without any of the sfs API
 * being implemented.
 */
int initDiskTest() {
    int hr = SUCCESS;

    FAIL_BRK4(initDisk());

    Fail:

    saveAndCloseDisk();
    PRINT_RESULTS("Init Disk Test (This test should not fail. Not worth points.)");
    return hr;
}

/**
 * Tests the initFS functionality.
 */
int initFSTest() {
    int hr = SUCCESS;

    FAIL_BRK4(initAndLoadDisk());
    FAIL_BRK4(initFS());

    Fail:

    saveAndCloseDisk();
    PRINT_RESULTS("Init FS Test");
    return hr;
}

/**
 * This is a place where you need to put your test. Please insert your test code here and
 * change the name of the test to be representative of what you are testing. You can use this test
 * during the development period to test your code. When you do the final submission here you have to
 * implement a test that tests some interesting functionality of the sfs.
 * @return SUCCESS if the test is passed successfully FAIL otherwise.
 */
int customTest() {
    int hr = SUCCESS;
	int i, j, fd, fd1, fd2;
    char *randomBuf;//buffer contain junk data, size: SD_SECTORSIZE
	randomBuf = (char *) malloc(sizeof(char) * SD_SECTORSIZE);
	
	// normal test from testfs
    // initialize disk
    FAIL_BRK4(SD_initDisk());
    for (i = 0; i < SD_NUMSECTORS; i++) {
        for (j = 0; j < SD_SECTORSIZE; j++) {
            randomBuf[j] = (char) rand();
        }
		while (SD_write(i, (void*) randomBuf));
    }
	FAIL_BRK4(SD_loadDisk(gsDiskFName));
	refreshDisk();

    // initialize fs, sfs_mkfs
    FAIL_BRK4(initFS());
	refreshDisk();
	
	// initialize test sfs_mkfs, when mkfs, nothing should appear again.
    FAIL_BRK4(initFS());
	FAIL_BRK4(sfs_mkdir("foo"));
	refreshDisk();
	FAIL_BRK4(sfs_fcd("foo"));
	FAIL_BRK4(sfs_mkdir("bar"));
	FAIL_BRK4(initFS());
	refreshDisk();
	FAIL_BRK4((sfs_fcd("foo") != -1));
	FAIL_BRK4((sfs_fcd("bar") != -1));
	FAIL_BRK4(sfs_fcd("/"));
	FAIL_BRK4((sfs_fcd("foo") != -1));
	FAIL_BRK4((sfs_fcd("bar") != -1));
	FAIL_BRK4(sfs_mkdir("foo"));
	refreshDisk();
	FAIL_BRK4(sfs_fcd("foo"));
	FAIL_BRK4((sfs_fcd("bar") != -1));

	//normal test . and ..
	FAIL_BRK4(initFS());
	refreshDisk();
	FAIL_BRK4((sfs_mkdir(".") != -1));
	FAIL_BRK4(sfs_mkdir(".foo."));
	FAIL_BRK4(sfs_mkdir(".foo"));
	FAIL_BRK4((sfs_mkdir("..") != -1));
	FAIL_BRK4(sfs_mkdir("..foo"));
	FAIL_BRK4(sfs_mkdir("..foo.."));
	FAIL_BRK4((sfs_mkdir("/") != -1));
	FAIL_BRK4(sfs_mkdir("..."));
	FAIL_BRK4(sfs_mkdir("...."));
	FAIL_BRK4((sfs_mkdir("./") != -1));
	FAIL_BRK4((sfs_mkdir("/.") != -1));
	FAIL_BRK4((sfs_mkdir("./.") != -1));
	
	// ascii code test, make a file containing all ascii code chars to make sure implementation does not use an EOF char for size
	FAIL_BRK4(initFS());
	refreshDisk();
	char* asciidata = (char*)malloc(257 * sizeof(char)); // 257 chars, possible ascii 0 -> 255 + 255 at the end to make sure we test
	// create data
	for (i = 0; i < 256; i++) {
		asciidata[i] = i; // table sequentially
	}
	// now put 255 in again at the end
	asciidata[256] = 255;
	// now fwrite into the a file and fread and compare
	FAIL_BRK4(createSmallFile("asciitable", asciidata, 257));
	refreshDisk();
	FAIL_BRK4(verifyFile("asciitable", asciidata, 257));
	
	//test dir takes more that one sector
	
	//test for file
	FAIL_BRK4(initFS());
	refreshDisk();
	FAIL_BRK4((fd = sfs_fopen("foo.bin")) == -1);
	FAIL_BRK4(sfs_fclose(fd));
	FAIL_BRK4((sfs_fcd("foo.bin") != -1));
	
	//test same name
	FAIL_BRK4(initFS());
	refreshDisk();
	
	FAIL_BRK4(sfs_mkdir("foo.bin"));
	FAIL_BRK4((fd = sfs_fopen("foo.bin")) != -1);
	
	FAIL_BRK4((fd = sfs_fopen("bar.bin")) == -1);
	FAIL_BRK4((sfs_mkdir("bar.bin") != -1));

	
/*	
    FAIL_BRK4(createFolder("bar"));
	FAIL_BRK4(sfs_fcd("bar"));
	FAIL_BRK4(createFolder("foo"));	
	FAIL_BRK4(sfs_fcd("foo"));
	FAIL_BRK4(sfs_fcd("/bar/foo"));
//	FAIL_BRK3((sfs_fcd("//bar/foo") != -1), stdout, "Error: sfs_fcd() failed\n");
	FAIL_BRK4(sfs_fcd("/../bar/foo"));
	FAIL_BRK4(sfs_fcd("/../bar/foo/"));
	FAIL_BRK4(sfs_fcd("/"));
	FAIL_BRK3(sfs_ls(f_ls), stdout, "Error: sfs_ls() failed\n");
	FAIL_BRK4(sfs_fcd("/bar"));
	FAIL_BRK3(sfs_ls(f_ls), stdout, "Error: sfs_ls() failed\n");
	
	//int fopentmp;
	//FAIL_BRK3((fopentmp = sfs_fopen("test.txt") != -1), stdout, "Error: Allowing read from unopened file\n");
	//FAIL_BRK3((sfs_fread(fopentmp, buf, 50) != -1), stdout, "Error: Allowing read from unopened file\n");

    // test if fcd does not allow going to places that does not exist
    FAIL_BRK3((sfs_fcd("bla") != -1), stdout,
            "Error: Allowing cd to folder that does not exist\n");
    FAIL_BRK3((sfs_fcd("x") != -1), stdout,
            "Error: Allowing cd to folder that does not exist\n");
    FAIL_BRK3((sfs_fcd("x/y/x/z") != -1), stdout,
            "Error: Allowing cd to folder that does not exist\n");
*/

    Fail:

    //clean up code goes here
	SAFE_FREE(asciidata);
	SAFE_FREE(randomBuf);
    saveAndCloseDisk();
    PRINT_RESULTS("customTest!");
    return hr;
}

/**
 * Tests the creation of a small file without attempting to read it afterwards.
 */
int createSimpleFileTest() {
    int hr = SUCCESS;
    int fsize = 250;
    char fdata[fsize];
    initBuffer(fdata, fsize);

    FAIL_BRK4(initAndLoadDisk());
    FAIL_BRK4(initFS());
    FAIL_BRK4(createSmallFile("foo", fdata, fsize));

    Fail:

    saveAndCloseDisk();
    PRINT_RESULTS("Create Simple File Test");
    return hr;
}

/**
 * Tests the file can also be read back and that contents match
 */
int createSimpleFileWriteReadTest() {
    int hr = SUCCESS;
    int fsize = 250;
    char fdata[fsize];
    initBuffer(fdata, fsize);

    FAIL_BRK4(initAndLoadDisk());
    FAIL_BRK4(initFS());
    FAIL_BRK4(createSmallFile("foo", fdata, fsize));
    FAIL_BRK4(saveAndCloseDisk());
    FAIL_BRK4(initAndLoadDisk());
    FAIL_BRK4(verifyFile("foo", fdata, fsize));

    Fail:

    saveAndCloseDisk();
    PRINT_RESULTS("Create Simple File Write Read Test");
    return hr;
}

/**
 * Tests a big file write and then read with various techniques.
 */
int singleBigFileTest() {
    int hr = SUCCESS;
    int fsize = SD_SECTORSIZE * 16;
    char *fileName = "foo";

    FAIL_BRK4(initAndLoadDisk());
    FAIL_BRK4(initFS());
    FAIL_BRK4(singleBigFile(fileName, fsize));

    Fail:

    saveAndCloseDisk();
    PRINT_RESULTS("Single Big File Test");
    return hr;
}

/**
 * Tests a number of files in the root folder.
 */
int multipleFilesTest() {
    int hr = SUCCESS;
    int numFiles = 50;
    int maxFileSize = 10 * SD_SECTORSIZE;

    FAIL_BRK4(initAndLoadDisk());
    FAIL_BRK4(initFS());
    FAIL_BRK4(multipleFiles("file", numFiles, maxFileSize));

    Fail:

    saveAndCloseDisk();
    PRINT_RESULTS("Multiple Files Test");
    return hr;
}

/**
 * Tests the creation of a simple folder.
 */
int createFolderTest() {
    int hr = SUCCESS;

    FAIL_BRK4(initAndLoadDisk());
    FAIL_BRK4(initFS());
    FAIL_BRK4(createFolder("bar"));

    Fail:

    saveAndCloseDisk();
    PRINT_RESULTS("Create Folder Test");
    return hr;
}

/**
 * Tests the creation of a number of folders in the root
 */
int multipleFoldersTest() {
    int hr = SUCCESS;

    FAIL_BRK4(initAndLoadDisk());
    FAIL_BRK4(initFS());
    FAIL_BRK4(createFolders("folder", 500));

    Fail:

    saveAndCloseDisk();
    PRINT_RESULTS("Multiple Folders Test");
    return hr;
}

/**
 * Tests the ability to add more data to a existing file.
 */
int appendFileTest() {
    int hr = SUCCESS;

    FAIL_BRK4(initAndLoadDisk());
    FAIL_BRK4(initFS());
    FAIL_BRK4(appendFile());

    Fail:

    saveAndCloseDisk();
    PRINT_RESULTS("Append File Test");
    return hr;
}

/**
 * Tests having multiple files openned and written at the same time
 */
int multipleOpenFilesTest() {
    int hr = SUCCESS;
    int fd, fd2, fd3, fsize = SD_SECTORSIZE;
    char *fname1 = "simult1";
    char *fname2 = "simult2";
    char *fname3 = "simult3";
    char *buffer = malloc(fsize);
    initBuffer(buffer, fsize);

    FAIL_BRK4(initAndLoadDisk());
    FAIL_BRK4(initFS());

    // Openning the files
    fd = sfs_fopen(fname1);
    FAIL_BRK3((fd == -1), stdout, "Error: fopen for (%s) failed\n", fname1);

    fd2 = sfs_fopen(fname2);
    FAIL_BRK3((fd2 == -1), stdout, "Error: fopen for (%s) failed\n", fname2);

    fd3 = sfs_fopen(fname3);
    FAIL_BRK3((fd3 == -1), stdout, "Error: fopen for (%s) failed\n", fname3);

    // Writing in a different order
    FAIL_BRK3((sfs_fwrite(fd2, buffer, fsize) != fsize), stdout,
            "Error: Write failed\n");
    FAIL_BRK3((sfs_fwrite(fd, buffer, fsize) != fsize), stdout,
            "Error: Write failed\n");
    FAIL_BRK3((sfs_fwrite(fd3, buffer, fsize) != fsize), stdout,
            "Error: Write failed\n");

    // Closing the files in different order
    FAIL_BRK3(sfs_fclose(fd3), stdout, "Error: Closing the file failed\n");
    FAIL_BRK3(sfs_fclose(fd2), stdout, "Error: Closing the file failed\n");
    FAIL_BRK3(sfs_fclose(fd), stdout, "Error: Closing the file failed\n");

    FAIL_BRK3(sfs_ls(f_ls), stdout, "Error: ls failed\n");

    // reload the disk and verify
    FAIL_BRK4(saveAndCloseDisk());
    FAIL_BRK4(initAndLoadDisk());
    FAIL_BRK4(verifyFile(fname1, buffer, fsize));
    FAIL_BRK4(verifyFile(fname2, buffer, fsize));
    FAIL_BRK4(verifyFile(fname3, buffer, fsize));

    Fail:

    SAFE_FREE(buffer);

    saveAndCloseDisk();
    PRINT_RESULTS("Multiple Open Files Test");
    return hr;
}

/**
 * Tests the nesting of folders
 */
int nestedFoldersTest() {
    int hr = SUCCESS;
    int numNestedFolders = 50;

    FAIL_BRK4(initAndLoadDisk());
    FAIL_BRK4(initFS());
    FAIL_BRK4(nestedFolders("dir", numNestedFolders));

    Fail:

    saveAndCloseDisk();
    PRINT_RESULTS("Nested Folders Test");
    return hr;
}

/**
 * Tests some error messages that we expect when we pass bogus arguments the sfs API
 */
int errorTest() {
    int hr = SUCCESS;
    char buf[100];

    // test setup
    FAIL_BRK4(initAndLoadDisk());
    FAIL_BRK4(initFS());

    // make sure we are not allowed to read if the files are not open
    FAIL_BRK3((sfs_fread(0, buf, 50) != -1), stdout,
            "Error: Allowing read from unopened file\n");
    FAIL_BRK3((sfs_fread(rand(), buf, 50) != -1), stdout,
            "Error: Allowing read from unopened file\n");
    FAIL_BRK3((sfs_fread(-5, buf, 50) != -1), stdout,
            "Error: Allowing read from unopened file\n");

    // test if fcd does not allow going to places that does not exist
    FAIL_BRK3((sfs_fcd("bla") != -1), stdout,
            "Error: Allowing cd to folder that does not exist\n");
    FAIL_BRK3((sfs_fcd("x") != -1), stdout,
            "Error: Allowing cd to folder that does not exist\n");
    FAIL_BRK3((sfs_fcd("x/y/x/z") != -1), stdout,
            "Error: Allowing cd to folder that does not exist\n");

    // testing if lseek does not allow going pass the end of the file
    int fsize = 10;
    FAIL_BRK3(createSmallFile("foo", "aaaaaaaaaaaaaaaaaaaaaaaa", fsize),
            stdout, "Error: Creating file failed\n");

    int fd = sfs_fopen("foo");
    FAIL_BRK3((fd == -1), stdout, "Error: reopening the file failed\n");

    FAIL_BRK3((sfs_lseek(fd, fsize) != -1), stdout,
            "Error: Allowing seek pass the end of the file\n");

    Fail:

    saveAndCloseDisk();
    PRINT_RESULTS("Error Test");
    return hr;
}

/**
 * Tests sfs_rm functionality.
 */
int removeTest() {
    int hr = SUCCESS;
    char dirName[16], fileName[16];
    int i, numDirs = 100, numFiles = 100, fsize = SD_SECTORSIZE * 1.8;
    char *buffer = malloc(fsize * sizeof(char));

    // test setup
    FAIL_BRK4(initAndLoadDisk());
    FAIL_BRK4(initFS());

    FAIL_BRK3(sfs_ls(f_ls), stdout, "Error: sfs_ls() failed\n");

    // make some files
    for (i = 0; i < numFiles; i++) {
        sprintf(fileName, "file%04d", i);

        initBuffer(buffer, fsize);
        FAIL_BRK4(createSmallFile(fileName, buffer, fsize));
    }

    // make some folders
    for (i = 0; i < numDirs; i++) {
        sprintf(dirName, "dir%04d", i);
        FAIL_BRK4(createFolder(dirName));
    }

    FAIL_BRK3(sfs_ls(f_ls), stdout, "Error: sfs_ls() failed\n");

    // delete the even folders and files
    for (i = 0; i < numFiles; i += 2) {
        sprintf(fileName, "file%04d", i);
        FAIL_BRK3(sfs_rm(fileName), stdout,
                "Error: deleting file (%s) failed\n", fileName);

        sprintf(dirName, "dir%04d", i);
        FAIL_BRK3(sfs_rm(dirName), stdout,
                "Error: deleting folder (%s) failed\n", dirName);
    }

    FAIL_BRK3(sfs_ls(f_ls), stdout, "Error: sfs_ls() failed\n");

    // delete the rest of the files and folders
    for (i = 1; i < numFiles; i += 2) {
        sprintf(fileName, "file%04d", i);
        FAIL_BRK3(sfs_rm(fileName), stdout,
                "Error: deleting file (%s) failed\n", fileName);

        sprintf(dirName, "dir%04d", i);
        FAIL_BRK3(sfs_rm(dirName), stdout,
                "Error: deleting folder (%s) failed\n", dirName);
    }

    FAIL_BRK3(sfs_ls(f_ls), stdout, "Error: sfs_ls() failed\n");

    Fail:

    SAFE_FREE(buffer);
    saveAndCloseDisk();
    PRINT_RESULTS("Remove Test");
    return hr;
}

/**
 * Tests the algorithm for performance
 */
int perfTest() {
    int hr = SUCCESS;
    int i;
    char *fileName = malloc(32);
    char *dirName = malloc(32);

    printf("###########################\n");
    printf("Let the competition begin!\n");

    FAIL_BRK4(initAndLoadDisk());
    FAIL_BRK4(initFS());

    // lots of files
    FAIL_BRK4(multipleFiles("file", 900, SD_SECTORSIZE));

    // deleting them
    for (i = 0; i < 900; i++) {
        sprintf(fileName, "file%05d", i);
        FAIL_BRK4(sfs_rm(fileName));
    }

    // lots of files again
    FAIL_BRK4(multipleFiles("file", 900, SD_SECTORSIZE));

    // now deleting them in reverse
    for (i = 900 - 1; i >= 0; i--) {
        sprintf(fileName, "file%05d", i);
        FAIL_BRK4(sfs_rm(fileName));
    }

    // one very big file
    FAIL_BRK4(singleBigFile("huge.txt", (SD_NUMSECTORS - 100) * SD_SECTORSIZE));
    // and now it is gone
    FAIL_BRK4(sfs_rm("huge.txt"));

    // lots of flat folders
    FAIL_BRK4(createFolders("dir", 900));
    for (i = 0; i < 900; i++) {
        sprintf(dirName, "dir%04d", i);
        FAIL_BRK4(sfs_rm(dirName));
    }

    // lots of nested folders
    FAIL_BRK4(nestedFolders("dir", 900));


    // going to the bottom
    FAIL_BRK3(sfs_fcd("/"), stdout, "Error: cd / failed\n");
    for (i = 0; i < 900; i++) {
        sprintf(dirName, "dir%04d", i);
        FAIL_BRK3(sfs_fcd(dirName), stdout , "Error: fcd to (%s) failed\n", dirName);
    }

    // deleting them on the way out
    for (i = 900 - 1; i >= 0; i--) {
        sprintf(dirName, "dir%04d", i);
        FAIL_BRK3(sfs_fcd(".."), stdout, "Error: cd .. failed\n");
        FAIL_BRK3(sfs_rm(dirName), stdout, "Error: rm (%s) failed\n", dirName);
    }


    Fail:

    SAFE_FREE(fileName);
    SAFE_FREE(dirName);
    printf("COMPETITION RESULTS: ");
    saveAndCloseDisk();
    PRINT_RESULTS("Performance Test");
    return hr;
}

/**
 * Initialize the Disk with random content.
 */
int initDisk(void) {
    int hr = SUCCESS;
    int i, j;
    char *randomBuf;

    // initialize disk
    LOG(stdout, "Initialize disk before using it\n");
    FAIL_BRK3(SD_initDisk(), stderr, "Problems initializing disk %d\n", sderrno);
    LOG(stdout, "Write random garbage to disk\n");

    randomBuf = (char *) malloc(sizeof(char) * SD_SECTORSIZE);
    for (i = 0; i < SD_NUMSECTORS; i++) {
        // fill the buffer with random junk
        for (j = 0; j < SD_SECTORSIZE; j++) {
            randomBuf[j] = (char) rand();
        }

        // continue trying to write for number of times
        int numTries = 10;
        while (SD_write(i, (void*) randomBuf) && numTries) {
            fprintf(stdout, "Error %d while writing block %d\n", sderrno, i);
            numTries--;
        }
        FAIL_BRK3((numTries == 0), stderr,
                "Final attempt to write %d block to disk FAILED.", i);

        LOG(stdout, "Block written (%d) \n", i);
    }

    Fail:

    SAFE_FREE(randomBuf);
    return hr;
}

/**
 * Save the disk to a file and finalize the driver.
 */
int saveAndCloseDisk() {
    int hr = SUCCESS;
    FAIL_BRK3(SD_saveDisk(gsDiskFName), stdout,
            "Error %d while saving disk image to %s\n", sderrno, gsDiskFName);
    LOG(stdout, "Disk image saved to %s\n", gsDiskFName);

    FAIL_BRK3(SD_finalizeDisk(), stdout,
            "Error %d while during SD_finalizeDisk()\n", sderrno);

    Fail: return hr;
}

/**
 * Initialize the disk and load the content from the file.
 */
int initAndLoadDisk() {
    int hr = SUCCESS;
    FAIL_BRK3(SD_initDisk(), stdout, "Error %d while SD_initDisk()\n", sderrno);
    FAIL_BRK3(SD_loadDisk(gsDiskFName), stdout,
            "Error %d while reading disk image from %s\n", sderrno, gsDiskFName);
    LOG(stdout, "Disk image restored from %s\n", gsDiskFName);

    Fail: return hr;
}

/**
 * Use sfs_mkfs() to initialize the filesystem
 */
int initFS() {
    int hr = SUCCESS;
    // initialize fs
    LOG(stdout, "Initialize file system...\n");
    FAIL_BRK3(sfs_mkfs(), stdout, "Error making file system...aborting\n");
    LOG(stdout, "File System created successfully ...\n");

    Fail: return hr;
}

/**
 * Opens a file, writes some data to it and closes it.
 */
int createSmallFile(char *fileName, char *data, int fsize) {
    int hr = SUCCESS;
    // create one file, write to it, read it back
    // open file
    int fd = sfs_fopen(fileName);
    FAIL_BRK3((fd == -1), stdout,
            "Error: Unable to open file %s with intention to write\n", fileName);
    // write file
    FAIL_BRK3((sfs_fwrite(fd, data, fsize) != fsize), stdout,
            "Error: failed to write %d bytes to %s, fd:%d\n", fsize, fileName,
            fd);
    // close file
    FAIL_BRK3(sfs_fclose(fd), stdout, "Error: Unable to close fd: %d\n", fd);

    Fail: return hr;
}

/**
 * Checks if the contents of the file match the give data.
 */
int verifyFile(char *fileName, char *data, int fsize) {
    int hr = SUCCESS;
    /* open a file and verify the contents of the file are correct */
    char data2[fsize];
    // open file
    int fd = sfs_fopen(fileName);
    FAIL_BRK3((fd == -1), stdout,
            "Error: Unable to open file %s with intention to read\n", fileName);
    // read file
    FAIL_BRK3((sfs_fread(fd, data2, fsize) != fsize), stdout,
            "Error: failed to read %d bytes to %s, fd:%d\n", fsize, fileName,
            fd);

    FAIL_BRK3(checkBuffers(data, data2, fsize, 0), stdout,
            "Error: contents didn't match\n");

    // TODO: We should not allow reading pass the end of the file...
    //int bread;
    //FAIL_BRK( ((bread = sfs_fread(fd, data2, 1)) == 1), stdout, "Error: successfully reading more data when we didn't expected it %d\n", bread);

    // close file
    FAIL_BRK3(sfs_fclose(fd), stdout, "Error: Unable to close fd: %d\n", fd);

    Fail: return hr;
}

/**
 * Test a single big file
 */
int singleBigFile(char *fileName, int fsize) {
    int hr = SUCCESS;

    FAIL_BRK3(testFile(fileName, fsize), stdout,
            "Error: testing one file...aborting\n");

    Fail: return hr;
}

/**
 * Creating a number of files given a base filename, number of files and the size
 */
int multipleFiles(char *fileBaseName, int numFiles, int maxFileSize) {
    int hr = SUCCESS;
    int fsize, i;
    char *fileName = malloc(strlen(fileBaseName) * sizeof(char) + 32);

    for (i = 0; i < numFiles; i++) {
        sprintf(fileName, "%s%05d", fileBaseName, i);
        fsize = rand() % maxFileSize;
        FAIL_BRK(testFile(fileName, fsize), stdout,
                "Error: File testing failed for (%s) size (%d)\n", fileName,
                fsize);
    }

    Fail:

    SAFE_FREE(fileName);
    return hr;
}

/**
 * Creates a single folder give a name and tries to cd into it and back out.
 */
int createFolder(char *dirName) {
    int hr = SUCCESS;

    FAIL_BRK3(sfs_mkdir(dirName), stdout,
            "Error: Creating folder (%s) failed\n", dirName);

    FAIL_BRK3(sfs_fcd(dirName), stdout,
            "Error: Attempt to cd to a new folder (%s) failed\n", dirName);

    FAIL_BRK3(sfs_fcd(".."), stdout, "Error: Attempt to cd back to .. failed\n");

    Fail: return hr;
}

/**
 * Create a number of folders
 */
int createFolders(char *dirBaseName, int numDirs) {
    int hr = SUCCESS;
    int i;
    char *dirName = malloc(strlen(dirBaseName) * sizeof(char) + 32);

    for (i = 0; i < numDirs; i++) {
        sprintf(dirName, "%s%04d", dirBaseName, i);
        FAIL_BRK4(createFolder(dirName));
    }

    Fail:

    SAFE_FREE(dirName);
    return hr;
}

/**
 * Create a big file, write lots of data to it and close it. Then append another chunk of data to it.
 */
int appendFile() {
    int hr = SUCCESS;
    int fsize1 = 20 * SD_SECTORSIZE;
    int fsize2 = 50 * SD_SECTORSIZE;
    char *buffer = malloc(fsize2 * sizeof(char));
    char *buffer2 = malloc(fsize2 * sizeof(char));

    char *fileName = "foo";

    // create the original file
    FAIL_BRK3(singleBigFile(fileName, fsize1), stdout,
            "Error: Unable to create the initial file\n");

    FAIL_BRK3(refreshDisk(), stdout, "Error: Refresh disk failed\n");

    // open it again
    int fd = sfs_fopen(fileName);
    FAIL_BRK3((fd == -1), stdout, "Error: Unable to reopen the file\n");

    // lseek to the end of the file
    int newPos = sfs_lseek(fd, fsize1 - 1);
    FAIL_BRK3(
            (newPos != fsize1 - 1),
            stdout,
            "Error: Seeking1 to the end of the file failed newPos (%d), fsize1(%d)\n",
            newPos, fsize1);

    initBuffer(buffer, fsize2);
    // write the new data
    int bytesWritten = sfs_fwrite(fd, buffer, fsize2);
    FAIL_BRK3((bytesWritten != fsize2), stdout,
            "Error: Appending write failed\n");

    // close the file
    FAIL_BRK3(sfs_fclose(fd), stdout, "Error: Closing the file failed\n");

    FAIL_BRK3(refreshDisk(), stdout, "Error: Refresh disk failed\n");

    // open it again
    fd = sfs_fopen(fileName);
    FAIL_BRK3((fd == -1), stdout, "Error: Unable to reopen the file\n");

    // again lseek to the place where we started appending
    newPos = sfs_lseek(fd, fsize1 - 1);
    FAIL_BRK3((newPos != fsize1 - 1), stdout,
            "Error: Seeking2 to the end of the file failed\n");

    // read the data
    int bytesRead = sfs_fread(fd, buffer2, fsize2);
    FAIL_BRK3((bytesRead != fsize2), stdout,
            "Error: Reading back the appended part failed\n");

    // make sure it matches
    FAIL_BRK3(checkBuffers(buffer, buffer2, fsize2, 0), stdout,
            "Error: Contents don't match\n");

    // close the file
    FAIL_BRK3(sfs_fclose(fd), stdout, "Error: Closing the file failed\n");

    // finalize
    Fail:

    // clean up
    SAFE_FREE(buffer);
    SAFE_FREE(buffer2);
    return hr;
}

/**
 * creates a number of nested folders
 */
int nestedFolders(char *dirBaseName, int num) {
    int hr = SUCCESS;
    int i;
    char *dirName = malloc(strlen(dirBaseName) + 32);

    for (i = 0; i < num; i++) {
        sprintf(dirName, "%s%04d", dirBaseName, i);
        FAIL_BRK3(sfs_mkdir(dirName), stdout,
                "Error: Creating folder (%s) failed\n", dirName);

        FAIL_BRK3(sfs_fcd(dirName), stdout,
                "Error: Changing folder to (%s) failed\n", dirName);
    }

    // going back to /
    FAIL_BRK3(sfs_fcd("/"), stdout, "Error: Going to root failed\n");

    // cd down one by one
    for (i = 0; i < num; i++) {
        sprintf(dirName, "dir%04d", i);
        FAIL_BRK3(sfs_fcd(dirName), stdout,
                "Error: Changing folder to (%s) failed\n", dirName);
    }

    // cd back up one by one
    for (i = 0; i < num; i++) {
        FAIL_BRK3(sfs_fcd(".."), stdout,
                "Error: Changing folder to (..) failed\n");
        if (i % 10 == 0) {
            FAIL_BRK3(sfs_ls(f_ls), stdout, "Error: ls failed ...");
        }
    }

    // cd down one by one
    for (i = 0; i < num; i++) {
        sprintf(dirName, "dir%04d", i);
        FAIL_BRK3(sfs_fcd(dirName), stdout,
                "Error: Changing folder to (%s) failed\n", dirName);
    }

    Fail:

    SAFE_FREE(dirName);
    return hr;
}

/**
 * Checks to see that you're writing through your cache eventually.
 *
 * @param diskFName name of disk file
 *
 */
int refreshDisk() {
    int hr = SUCCESS;

    FAIL_BRK3(SD_saveDisk(gsDiskFName), stdout,
            "Error %d while saving disk image to %s\n", sderrno, gsDiskFName);

    FAIL_BRK3(SD_loadDisk(gsDiskFName), stdout,
            "Error %d while reading disk image from %s\n", sderrno, gsDiskFName);

    Fail: return hr;
}

/**
 * Loads buffer with random data.
 *
 * @param buf buffer
 * @param fsize size of buffer
 *
 */
void initBuffer(char *buf, int fsize) {
    int i;
    for (i = 0; i < fsize; i++)
        buf[i] = (char) rand();
}

/**
 * Checks that two buffers are identical.
 *
 * @param buf first buffer
 * @param cpy second buffer
 * @param fsize size of buffer
 * @param offset point at which to start
 * @return -1 upon mismatch, 0 otherwise
 */
int checkBuffers(char *buf, char *cpy, int fsize, int offset) {
    int i;

    for (i = offset; i < fsize; i++) {
        if (buf[i] != cpy[i]) {
            LOG(stdout, "Buffer do not match at byte %d\n", i);
            fflush(stdout);
            return -1;
        }
    }
    LOG(stdout, "Success. Buffers match\n");
    return 0;
}

/**
 * Performs various file operations.
 *
 * @param name name of file to create
 * @param fsize size of file to create
 * @return -1 upon failure, 0 otherwise
 */
int testFile(char* name, int fsize) {
    int fd = 0; /* file handle */
    int bytes, bytesToRead, offset;
    // fix fsize of too small
    if (fsize<=1) fsize=2;
    char *buf = (char *) malloc(sizeof(char) * fsize);
    char *cpy = (char *) malloc(sizeof(char) * fsize);

    LOG(stdout, "Create file...\n");

    fd = sfs_fopen(name);
    FAIL_BRK((fd == -1), stdout, "Error: Open file failed for: %s\n", name);

    initBuffer(buf, fsize);

    /* write to disk */
    FAIL_BRK((sfs_fwrite(fd, buf, fsize) == -1), stdout,
            "Error: Writing %d bytes to file %s failed!\n", fsize, name);

    /* close file */
    FAIL_BRK(sfs_fclose(fd), stdout, "Closing file %s failed!\n", name);

    /* save disk, close it and reload it to make sure changes are flushed to disk */
    FAIL_BRK(refreshDisk(), stdout, "Error: Refreshing disk failed\n");

    /* open file again */
    fd = sfs_fopen(name);
    FAIL_BRK((fd == -1), stdout, "Error: Second open file failed for: %s\n",
            name);

    /* read file */
    int fsize2;
    FAIL_BRK(((fsize2 = sfs_fread(fd, cpy, fsize)) == -1), stdout,
            "Read failed for: %s\n", name);

    FAIL_BRK(
            (fsize != fsize2),
            stdout,
            "Error: The amount we wrote (%d) is different from what we read (%d)\n",
            fsize, fsize2);

    /* check contents */
    FAIL_BRK(checkBuffers(buf, cpy, fsize, 0), stdout,
            "Error: Contents don't match\n");

    /* reset file */
    FAIL_BRK(sfs_fclose(fd), stdout, "Error: Closing file (%d)(%s) failed\n",
            fd, name);
    fd = sfs_fopen(name);
    FAIL_BRK((fd == -1), stdout, "Error: Opening file (%s) failed\n", name);

    free(cpy);
    cpy = (char *) malloc(sizeof(char) * fsize);

    /* check contents using small chunks */
    bytes = 0;
    while (bytes < fsize) {
        bytesToRead = rand() % 9 + 1;

        if (bytes + bytesToRead >= fsize) {
            bytesToRead = fsize - bytes;
        }

        /* read */
        int bytesActuallyRead;
        bytesActuallyRead = sfs_fread(fd, cpy + bytes, bytesToRead);
        FAIL_BRK((bytesActuallyRead == -1), stdout,
                "Error: Read failed for: %s\n", name);
        FAIL_BRK(
                (bytesActuallyRead != bytesToRead),
                stdout,
                "Error: For file (%s) expected to read (%d) but actually read (%d)\n",
                name, bytesToRead, bytesActuallyRead);

        bytes += bytesToRead;
    }

    FAIL_BRK(checkBuffers(buf, cpy, fsize, 0), stdout,
            "Error: Contents don't match\n");

    /* reset file */
    FAIL_BRK(sfs_fclose(fd), stdout, "Error: Closing file (%d) failed\n", fd);
    fd = sfs_fopen(name);
    FAIL_BRK((fd == -1), stdout, "Error: Opening file (%s) failed\n", name);

    free(cpy);
    cpy = (char *) malloc(sizeof(char) * fsize);

    /* check contents from offset */
    offset = rand() % (fsize - 1);
    bytesToRead = rand() % (fsize - offset);

    if (bytesToRead == 0)
        bytesToRead++;

    FAIL_BRK((sfs_lseek(fd, offset) == -1), stdout,
            "Error: lseek failed for: (%s) fd: (%d), offset: (%d)\n", name, fd,
            offset);

    /* read */
    int bytesActuallyRead = 0;
    bytesActuallyRead = sfs_fread(fd, cpy + offset, bytesToRead);
    FAIL_BRK((bytesActuallyRead == -1), stdout, "Error: Read failed for: %s\n",
            name);
    FAIL_BRK(
            (bytesActuallyRead != bytesToRead),
            stdout,
            "Error: For file (%s) expected to read (%d) but actually read (%d)\n",
            name, bytesToRead, bytesActuallyRead);

    FAIL_BRK(checkBuffers(buf, cpy, bytesToRead, offset), stdout,
            "Error: Contents do not match\n");

    FAIL_BRK(sfs_fclose(fd), stdout, "Error: Closing file (%d) failed\n", fd);

    SAFE_FREE(buf);
    SAFE_FREE(cpy);

    return 0; /* passed all tests */

    // Something went wrong...
    Fail:
    
    SAFE_FREE(buf);
    SAFE_FREE(cpy);
    
    return -1;
}

