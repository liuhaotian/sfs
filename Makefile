###############################################################################
#
# File:         Makefile
# RCS:          $Id: Makefile,v 1.2 2009/11/12 09:19:51 jot836 Exp $
# Description:  Simple File System
# Author:       Fabian E. Bustamante
#               Northwestern Systems Research Group
#               Department of Computer Science
#               Northwestern University
# Created:      Thu Nov 07, 2002 at 15:24:55
# Modified:     Wed Nov 17, 2004 at 14:48:27 fabianb@cs.northwestern.edu
# Language:     Makefile
# Package:      N/A
# Status:       Experimental (Do Not Distribute)
#
# (C) Copyright 2002, Northwestern University, all rights reserved.
#
###############################################################################

# handin info
TEAM = hlc720+gjw571
VERSION = `date +%Y%m%d%H%M%S`
PROJ = sfs

CC = gcc
MV = mv
CP = cp
RM = rm
MKDIR = mkdir
TAR = tar cvf
COMPRESS = gzip
CFLAGS = -Wall -g -D_GNU_SOURCE 
#CFLAGS = -Wall -g -D_GNU_SOURCE -DSD_WITHERROR

DELIVERY = Makefile sfs.c sfs.h testfs.c DOC TEAMNAME
PROGS = testfs testfs-ec testfs-compTest
SRCS_SD = sdisk.c sfs.c testfs.c
SRCS_FS = sdisk.c sfs.c testfs.c
OBJS_SD = ${SRCS_SD:.c=.o}
OBJS_FS = ${SRCS_FS:.c=.o}

all: ${PROGS}

test-reg: handin
	HANDIN=`pwd`/${TEAM}-${VERSION}-${PROJ}.tar.gz;\
	cd testsuite;\
	sh ./run_testcase.sh $${HANDIN};

handin: cleanAll
	echo ${TEAM} > TEAMNAME
	${TAR} ${TEAM}-${VERSION}-${PROJ}.tar ${DELIVERY}
	${COMPRESS} ${TEAM}-${VERSION}-${PROJ}.tar
	rm TEAMNAME

.o:
	${CC} *.c

testsd: ${SRCS_SD}
	${CC} ${CFLAGS} -o $@ ${SRCS_SD}

testfs: ${SRCS_FS}
	${CC} ${CFLAGS} -o $@ ${SRCS_FS}

testfs-ec: ${SRCS_FS}
	${CC} ${CFLAGS} -DEXTRA_CREDIT -o $@ ${SRCS_FS}

testfs-compTest: ${SRCS_FS}
	${CC} ${CFLAGS} -DCOMPETITION_TEST -o $@ ${SRCS_FS}

leak: all
	valgrind -v --tool=memcheck --show-reachable=yes --leak-check=yes ./testfs -f test.dat; \
	rm test.dat
	valgrind -v --tool=memcheck --show-reachable=yes --leak-check=yes ./testfs-ec -f test.dat; \
	rm test.dat
	valgrind -v --tool=memcheck --show-reachable=yes --leak-check=yes ./testfs-compTest -f test.dat; \
	rm test.dat

clean:
	${RM} -f *.o *~
	${RM} -f ${PROGS}
	${RM} -f *.ls

cleanAll: clean
	${RM} -f ${PROGS} ${TEAM}-${VERSION}-${PROJ}.tar.gz
