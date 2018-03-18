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


#if defined(unix) || defined(__unix) || defined (__unix__)
/* Code for Unix.  Any Unix compiler should define one of the above three
 * symbols. */

#ifndef startupSocket
#define startupSocket()
#endif

#ifndef closesocket
#define closesocket close
#endif

#ifndef write_socket
#define write_socket(r, s, l) write(r, s, l)
#endif

/* end of 'if unix' */

#elif defined(WIN32) || defined(__WIN32__)

#include <winsock2.h>  /* For NT socket */
#include <ws2tcpip.h>  /* IP_ADD_MEMBERSHIP */
#include <windows.h>
#include <stdio.h>     /* stderr */
#include <time.h>      /* time_t */

#define HAVE_STDINT_H 1
#define RTP_LITTLE_ENDIAN 1

/* Determine if the C(++) compiler requires complete function prototype  */
#ifndef __P
#if defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus)
#define __P(x) x
#else
#define __P(x) ()
#endif
#endif

#ifdef __BORLANDC__
#include <io.h>
#define strcasecmp stricmp
#define strncasecmp strnicmp
#endif /* __BORLANDC__ */

#ifdef _MSC_VER
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#define open _open
#define write _write
#define close _close
#define ftime _ftime
#define timeb _timeb
#endif /* _MSC_VER */

#ifndef SIGBUS
#define SIGBUS SIGINT
#endif

#ifndef SIGHUP
#define SIGHUP SIGINT
#endif

#ifndef SIGPIPE
#define SIGPIPE SIGINT
#endif

#ifndef EADDRNOTAVAIL
#define EADDRNOTAVAIL WSAEADDRNOTAVAIL
#endif

typedef SSIZE_T ssize_t;

typedef  INT32 pid_t;
typedef UINT32 gid_t;
typedef UINT32 uid_t;

typedef char *   caddr_t;        /* core address */
typedef long  fd_mask;
#define NBBY  8   /* number of bits in a byte */
#define NFDBITS (sizeof(fd_mask) * NBBY)  /* bits per mask */

#ifndef howmany
#define howmany(x, y) (((x) + ((y) - 1)) / (y))
#endif

struct iovec {
    void  *iov_base;    /* Starting address */
    size_t iov_len;     /* Number of bytes to transfer */
};

struct msghdr {
        caddr_t msg_name;               /* optional address */
        int     msg_namelen;            /* size of address */
        struct  iovec *msg_iov;         /* scatter/gather array */
        int     msg_iovlen;             /* # elements in msg_iov */
        caddr_t msg_accrights;          /* access rights sent/received */
        int     msg_accrightslen;
};

struct passwd {
        char    *pw_name;
        char    *pw_passwd;
        uid_t   pw_uid;
        gid_t   pw_gid;
        char    *pw_age;
        char    *pw_comment;
        char    *pw_gecos;
        char    *pw_dir;
        char    *pw_shell;
};

#define  ITIMER_REAL     0       /* Decrements in real time */

#ifndef _TIMESPEC_T
#define _TIMESPEC_T
typedef struct  timespec {              /* definition per POSIX.4 */
        time_t          tv_sec;         /* seconds */
        long            tv_nsec;        /* and nanoseconds */
} timespec_t;
#endif  /* _TIMESPEC_T */

struct  itimerval {
        struct  timeval it_interval;    /* timer interval */
        struct  timeval it_value;       /* current value */
};

#ifndef timerisset
#define timerisset(tvp)        ((tvp)->tv_sec || (tvp)->tv_usec)
#endif

#ifndef timerclear
#define timerclear(tvp)        ((tvp)->tv_sec = (tvp)->tv_usec = 0)
#endif

#ifndef timercmp
#define timercmp(a, b, CMP)                                                  \
  (((a)->tv_sec == (b)->tv_sec) ?                                             \
   ((a)->tv_usec CMP (b)->tv_usec) :                                          \
   ((a)->tv_sec CMP (b)->tv_sec))
#endif

#ifndef timeradd
#define timeradd(a, b, result)                                               \
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
#define timersub(a, b, result)                                               \
  do {                                                                        \
    (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;                             \
    (result)->tv_usec = (a)->tv_usec - (b)->tv_usec;                          \
    if ((result)->tv_usec < 0) {                                              \
      --(result)->tv_sec;                                                     \
      (result)->tv_usec += 1000000;                                           \
    }                                                                         \
  } while (0)
#endif

#ifndef ETIME
#define ETIME 1
#endif

#ifndef SIGKILL
#define SIGKILL SIGTERM
#endif

#define fork() 0
#define setsid() {}

#ifndef FILE_SOCKET
#define FILE_SOCKET int
#endif

#ifndef fdopen_socket
#define fdopen_socket(f, g) &f
#endif

#ifndef fclose_socket
#define fclose_socket(f) closesocket(*f)
#endif

extern int winfd_dummy;
extern char getc_socket(FILE_SOCKET *f);
extern ssize_t write_socket(int fildes, const void *buf, size_t nbyte);
extern int sendmsg(int s, const struct msghdr *msg, int flags);

/* end of 'ifdef WIN32' */
#else
#error "Not Unix or WIN32 -- what system is this?"
#endif

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#if !defined(sun4) && !defined(hp) && !defined(nextstep) && !defined(linux)
#include <sys/select.h>  /* select() */
#endif

#endif /* end of ifdef SYSDEP_H */
