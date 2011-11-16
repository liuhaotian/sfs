/* -*-C-*-
 *******************************************************************************
 *
 * File:         config.h
 * RCS:          $Id: config.h,v 1.3 2009/11/12 09:19:51 jot836 Exp $
 * Description:  Simple File System Interface
 * Author:       Nikola P Borisov
 *               Northwestern Systems Research Group
 *               Department of Computer Science
 *               Northwestern University
 * Created:      Tue Nov 05, 2002 at 07:40:42
 * Modified:     Fri Nov 19, 2004 at 15:45:16 fabianb@cs.northwestern.edu
 * Language:     C
 * Package:      N/A
 * Status:       Experimental (Do Not Distribute)
 *
 * (C) Copyright 2003, Northwestern University, all rights reserved.
 *
 *******************************************************************************
 */

#ifndef CONFIG_H
#define CONFIG_H

// bool support in C
#define bool short
#define TRUE 1
#define FALSE 0

#define SUCCESS 0                           // test success
#define FAIL -1                             // test fail
#define EXIT_ON_FIRST_FAIL FALSE            // should the test suite quit on the first failure
#define LOG(...) if (gbIsVerbose) { fprintf(__VA_ARGS__); }
#define FAIL_BRK(f,...) if (f) { fprintf(__VA_ARGS__); goto Fail;}
#define FAIL_BRK2(f) if (f) { goto Fail;}
#define FAIL_BRK3(f,...) if ( (hr = (f)) != SUCCESS ) { fprintf(__VA_ARGS__); goto Fail;}
#define FAIL_BRK4(f) if ( (hr = (f)) != SUCCESS ) { goto Fail;}
#define PRINT_RESULTS(testName) printf("%s: %s\n", ((hr == SUCCESS)? "PASS": "FAIL"), testName);
#define SAFE_FREE(p) if (p != NULL) { free(p); p = NULL; }
#define RUN_TEST(t) if ( (t) != SUCCESS && EXIT_ON_FIRST_FAIL) { hr = FAIL; goto Fail; }

#endif
