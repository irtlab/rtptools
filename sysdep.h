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

#ifndef SYSDEP_H
#define SYSDEP_H

/* In this file, we basically decide whether we are on Windows or not.
 * On Windows, define a bunch of stuff that Windows needs defined
 * (TODO: it could probably be cleand up a bit); otherwise,
 * simply include the config.h produced by configure. */

#if defined(WIN32) || defined(__WIN32__)

#define HAVE_ERR		0
#define HAVE_GETOPT		0
#define HAVE_GETTIMEOFDAY	0
#define HAVE_PROGNAME		0
#define HAVE_STRTONUM		0
#define HAVE_LNSL		0
#define HAVE_LSOCKET		0
#define HAVE_BIGENDIAN		0
#define HAVE_MSGCONTROL		0
#define RTP_BIG_ENDIAN		0

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>

#define strcasecmp _stricmp
#define strncasecmp _strnicmp

#ifndef SIGHUP
#define SIGHUP SIGINT
#endif

typedef uint32_t      in_addr_t;

struct iovec {
    void  *iov_base;    /* Starting address */
    size_t iov_len;     /* Number of bytes to transfer */
};

struct msghdr {
        char*   msg_name;               /* optional address */
        int     msg_namelen;            /* size of address */
        struct  iovec *msg_iov;         /* scatter/gather array */
        int     msg_iovlen;             /* # elements in msg_iov */
        char*   msg_accrights;          /* access rights sent/received */
        int     msg_accrightslen;
};

#define  ITIMER_REAL     0       /* Decrements in real time */

extern int sendmsg(int s, const struct msghdr *msg, int flags);
extern void startupSocket(void);

/* declare the missing functions */

extern void		err(int, const char *, ...);
extern void		errx(int, const char *, ...);
extern void		warn(const char *, ...);
extern void		warnx(const char *, ...);

extern char*		optarg;
extern int		opterr;
extern int		optind;
extern int		optopt;
extern int		optreset;
extern int		getopt(int, char* const*, const char*);

extern int		gettimeofday(struct timeval*, void*);

extern const char*	getprogname(void);
extern void		setprogname(const char *);

/* Win is missing these in #include <sys/time.h> */
#ifndef timeradd
#define timeradd(a, b, result)                                                \
  do {                                                                        \
    (result)->tv_sec = (a)->tv_sec + (b)->tv_sec;                             \
    (result)->tv_usec = (a)->tv_usec + (b)->tv_usec;                          \
    if ((result)->tv_usec >= 1000000)                                         \
      {                                                                       \
        ++(result)->tv_sec;                                                   \
        (result)->tv_usec -= 1000000;                                         \
      }                                                                       \
  } while (0)
#endif

#ifndef timersub
#define timersub(a, b, result)                                                \
  do {                                                                        \
    (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;                             \
    (result)->tv_usec = (a)->tv_usec - (b)->tv_usec;                          \
    if ((result)->tv_usec < 0) {                                              \
      --(result)->tv_sec;                                                     \
      (result)->tv_usec += 1000000;                                           \
    }                                                                         \
  } while (0)
#endif

#else /* not WIN32 */

#include "config.h"

/* Windows needs to call this function as the first thing
 * to init its socket stack as described in <winsock2.h>.
 * Use it uniformly in the code, but define it away if
 * we are not on Windows. */
#define startupSocket()

#endif

#endif /* SYSDEP_H */
