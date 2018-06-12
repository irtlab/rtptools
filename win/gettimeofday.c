/*
 * (c) 1998-2018 by Columbia University; all rights reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/timeb.h>
#include "sysdep.h"
#include "gettimeofday.h"

#if defined(WIN32)
int gettimeofday(struct timeval *tv, void *t)
{
  static struct timeval stv;    /* initial timeval */
  static INT64 sct, tick;
  static int initialized = -1;
  LARGE_INTEGER startCount;  
  LARGE_INTEGER tickPerSec;
  struct timeb tb;

  LARGE_INTEGER count;
  INT64 c;

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
