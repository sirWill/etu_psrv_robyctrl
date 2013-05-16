#ifndef DECLARATIONS_H_
#define DECLARATIONS_H_

#include <cstdlib>
#include <iostream>
#include <pthread.h>
#include <process.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include <sys/neutrino.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/siginfo.h>
#include <termios.h>

#define NSEC 5000000
//#define INT_NSEC 165000000
//#define INT_NSECF 165000000
#define INT_NSEC 5036898//интервал для таймера W - 2000
#define INT_NSECF 7517070//интервал для таймера F - 1000

#endif /* DECLARATIONS_H_ */
