/***************************************************************************
 *
 * Code file for gettimeofday() function.
 *  gettimeofday.h
 *
 *  Copyright 2001 by Columbia University; all rights reserved
 *  by Akira Tsukamoto
 *
 ***************************************************************************/

#include <stdio.h>
#include <sys\timeb.h>
#include "sysdep.h"
#include "gettimeofday.h"

#if defined(WIN32)
int gettimeofday(struct timeval *tv, void *t)
{
  static struct timeval stv;    /* initial timeval */
  static LONGLONG sct, tick;
  static int initialized = -1;
  LARGE_INTEGER startCount;  
  LARGE_INTEGER tickPerSec;
  struct timeb tb;

  LARGE_INTEGER count;
  LONGLONG c;

  if (initialized == -1) {
    ftime(&tb);
    stv.tv_sec  = tb.time;
    stv.tv_usec = tb.millitm * 1000;
    if (QueryPerformanceFrequency(&tickPerSec) == FALSE) {
      fprintf(stderr, "QueryPerformanceFrequency(): No high-reso counter.\n");
      exit(1);
    }
    if (QueryPerformanceCounter(&startCount) == FALSE) {
      fprintf(stderr, "QueryPerformanceCounter(): No high-reso counter.\n");
      exit(1);
    }
    sct = startCount.QuadPart; tick = tickPerSec.QuadPart;
    initialized = 0;
#ifdef DEBUG
    printf("init: gettimeofday: %ld %06ld\n", stv.tv_sec, stv.tv_usec);
#endif
  }

  if (!QueryPerformanceCounter(&count))
    return -1;
  c = count.QuadPart;
  tv->tv_sec  = stv.tv_sec  + (long)((c - sct) / tick);
  tv->tv_usec = stv.tv_usec + (long)(((c - sct) % tick) * 1000000 / tick);
  if (tv->tv_usec >= 1000000) {
    tv->tv_sec++;
    tv->tv_usec -= 1000000;
  }
#ifdef DEBUG
  printf("gettimeofday: %ld %06ld\n", tv->tv_sec, tv->tv_usec);
#endif
  return 0;
}
#endif /* WIN32 */

 
