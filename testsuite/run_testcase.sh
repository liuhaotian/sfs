#!/bin/sh

. ./config.test;

if [ ! "$#" -eq 1 ]; then
	echo -e "usage: $0 targz_file";
	exit 1;
fi;

TARGZ=$1;
TC_DIR=`pwd`;
#TMP=`mktemp -d /tmp/cs343.tests.XXXXXX`;
TMP=/tmp/cs343.tests.`whoami`.`date +%Y%m%d%H%M%S`.tmp;
mkdir ${TMP};
#OUTPUT=`mktemp -d /tmp/cs343.tests.XXXXXX`;
OUTPUT=/tmp/cs343.tests.`whoami`.`date +%Y%m%d%H%M%S`.out;
mkdir -p ${OUTPUT};
chmod go-rwx ${TMP} ${OUTPUT} || exit 1;
cp ${TARGZ} ${TMP}

cleanUp ()
{
	cd;
	rm -Rf ${TMP} ${OUTPUT};
}

echo "Testing ${TARGZ}";
echo;

# Untar sources
echo "UNTAR";
cd ${TMP} || { cleanUp; exit 1; }
gunzip -c ${TARGZ} | tar xvf - || { cleanUp; exit 1; }

echo;

if [ `ls *.c |wc -w` -eq 0 ]; then
    echo "error: no source files (*.c) found";
	echo "Please follow the submission instructions carefully.";
	cleanUp;
    exit 1;
fi;

# Setup the environment
echo "SETUP";

for f in ${ORIG_FILES}; do
	cp -f ${TC_DIR}/$f .;
done	

if [ ! -f Makefile -a ! -f makefile ]; then
	echo "warning: Makefile is missing";
fi;

echo;

# Compile the code
echo "COMPILE"

make >> ${OUTPUT}/gcc.output 2>&1;
echo "----------" >> ${OUTPUT}/gcc.output;

WARNING=`grep -c warning ${OUTPUT}/gcc.output`
ERROR=`grep -c error ${OUTPUT}/gcc.output`

echo "${WARNING} warning(s) found while compiling"
echo "${ERROR} error(s) found while compiling"

if [ ${WARNING} -gt 0 -o ${ERROR} -gt 0 ]; then
	echo;
	echo "GCC OUTPUT";
	cat ${OUTPUT}/gcc.output;
fi;

if [ ${ERROR} -gt 0 ]; then
	echo "error: failed to create executable";
	cat ${OUTPUT}/gcc.output;
	cleanUp;
	exit 1;
fi;

echo;

# Testing
echo "TESTING";

echo testfs -f temp.dat;
rm -f temp.dat
./testfs -f temp.dat > ${OUTPUT}/testfs.out 2>&1;
rm -f temp.dat
cat ${OUTPUT}/testfs.out

PASSED=`grep -c "PASS:" ${OUTPUT}/testfs.out`
FAILED=`grep -c "FAIL:" ${OUTPUT}/testfs.out`

echo;

echo "${PASSED} test cases passed."
echo "${FAILED} test cases failed."

echo;

# Testing basic tests LS output
echo "BASIC TEST LS OUTPUT COMPARISON";

diff --brief ${TC_DIR}/ref.base.ls base.ls
if [ $? -ne 0 ]; then
    echo "-- HOW IT SHOULD BE ------------------------------------------------------------ YOUR PROGRAM --------------------------------------------------------------";
    diff --side-by-side --width=160 ${TC_DIR}/ref.base.ls base.ls
    echo "------------------------------------------------------------------------------------------------------------------------------------------------------------";
    echo;
    echo "ls output does not match the expected output."
    echo "This may impact the number of tests that actually count as passing."
else
    echo "okay. ls output matches."
fi;

echo;

# Testing Extra Credit
echo "EXTRA CREDIT";

echo testfs-ec -f temp.dat;
rm -f temp.dat
./testfs-ec -f temp.dat > ${OUTPUT}/testfs.ec.out 2>&1;
rm -f temp.dat
cat ${OUTPUT}/testfs.ec.out

PASSED_EC=`grep -c "PASS:" ${OUTPUT}/testfs.ec.out`
FAILED_EC=`grep -c "FAIL:" ${OUTPUT}/testfs.ec.out`

echo;

echo "${PASSED_EC} extra credit test cases passed."
echo "${FAILED_EC} extra credit test cases failed."

echo;

# Testing basic tests LS output
echo "EXTRA CREDIT TEST LS OUTPUT COMPARISON";

diff --brief ${TC_DIR}/ref.extra_credit.ls extra_credit.ls
if [ $? -ne 0 ]; then
    echo "-- HOW IT SHOULD BE ------------------------------------------------------------ YOUR PROGRAM --------------------------------------------------------------";
    diff --side-by-side --width=160 ${TC_DIR}/ref.extra_credit.ls extra_credit.ls
    echo "------------------------------------------------------------------------------------------------------------------------------------------------------------";
    echo;
    echo "ls output does not match the expected output."
    echo "This may impact the number of tests that actually count as passing."
else
    echo "okay. ls output matches."
fi;

echo;

# Testing
echo "TESTING YOUR COMPETITION TEST";

echo testfs-compTest -f temp.dat;
rm -f temp.dat
./testfs-compTest -f temp.dat > ${OUTPUT}/testfs.compTest.out 2>&1;
rm -f temp.dat
cat ${OUTPUT}/testfs.compTest.out

PASSED=`grep -c "PASS:" ${OUTPUT}/testfs.compTest.out`
FAILED=`grep -c "FAIL:" ${OUTPUT}/testfs.compTest.out`

echo;

echo "${PASSED} test cases passed."
echo "${FAILED} test cases failed."

echo;

# Testing basic tests LS output
echo "COMPETITION TEST LS OUTPUT";
cat compTest.ls;
echo "-----------";

echo;

# Check for memory leaks
echo "CHECK FOR MEMORY LEAKS";
if [ `which valgrind 2>&1 | grep -c "no valgrind"` -gt 0 ]; then
	echo "error: valgrind not in PATH";
	echo "error: skip memory leak check";
else
	POS_LEAKS=0;
	DEF_LEAKS=0;
	STILL_REACHABLE=0;

	for f in ${PROGS}; do
	        make leak > ${OUTPUT}/testfs.valgrind.output 2>&1;
		valgrind -v --tool=memcheck --show-reachable=yes --leak-check=yes ./$f > ${OUTPUT}/${tc}.valgrind 2>&1;
	
		if [[ -n ${VERBOSE} ]]; then
			cat ${OUTPUT}/testfs.valgrind.output;
		fi;

		NOCHECK=`grep -c "no leaks are possible" ${OUTPUT}/testfs.valgrind.output`;
		if [[ ${NOCHECK} -eq 0 ]]; then
			leaks=`grep "possibly lost:" ${OUTPUT}/testfs.valgrind.output | sed "s/.*bytes in[[:space:]]*//" |sed "s/[[:space:]]*blocks.*//";`
			if [[ ${leaks} -gt ${POS_LEAKS} ]]; then
				POS_LEAKS=${leaks};
			fi;
			leaks=`grep "definitely lost:" ${OUTPUT}/testfs.valgrind.output | sed "s/.*bytes in[[:space:]]*//" |sed "s/[[:space:]]*blocks.*//";`
			if [[ ${leaks} -gt ${DEF_LEAKS} ]]; then
				DEF_LEAKS=${leaks};
			fi;
			leaks=`grep "still reachable:" ${OUTPUT}/testfs.valgrind.output | sed "s/.*bytes in[[:space:]]*//" |sed "s/[[:space:]]blocks.*//";`
			if [[ ${leaks} -gt ${DEF_LEAKS} ]]; then
				STILL_REACHABLE=${leaks};
			fi;
		fi;
	done;

	echo "${POS_LEAKS} possible leaks";
	echo "${DEF_LEAKS} leaks";
	echo "${STILL_REACHABLE} still reachable";
	echo;
fi;

# Competition
#echo "COMPETITION (SEED=113,NUM_OPS=24000,PROB_ALLOC=0.6,MAX_SIZE=8000,PROB_OVERSIZED=0.02)"

#for f in ${ORIG_COMPETITION}; do
#	cp -f ${TC_DIR}/$f .;
#done

# Compile the code
#echo "COMPILE"
#for f in ${PROGS}; do
#	echo "compiling $f";
#	FILES="";
#	for src in ${SRCS}; do
#		if [ -f ${src} ]; then
#			FILES="$FILES ${src}";
#		fi;
#	done;
#    ${CC} ${CFLAGS} -D${f} -o $f ${FILES};
#done
#
#echo;
#
#for f in ${PROGS}; do
#	echo $f;
#	time -p ./$f 2>&1;
#	echo;
#done
	
#Clean up
cleanUp;
