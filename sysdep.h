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

#elif defined(WIN32)


#define NOLONGLONG
#define RTP_LITTLE_ENDIAN 1
#define nextstep

#include <winsock2.h> /* For NT socket */
#include <sys/timeb.h> /* For _ftime() */
#include <sol_search.h> /* ENTRY, hsearch() */
#include <nterrno.h>
#include <utilNT.h> /* For function and struct in UNIX but not in NT */

#ifndef strcasecmp
#define strcasecmp _stricmp
#endif

#ifndef strncasecmp
#define strncasecmp _strnicmp
#endif

#ifndef open
#define open _open
#endif

#ifndef write
#define write _write
#endif

#ifndef close
#define close _close
#endif

#ifndef SIGBUS
#define SIGBUS SIGINT
#endif

#ifndef SIGHUP
#define SIGHUP SIGINT
#endif

#ifndef SIGPIPE
#define SIGPIPE SIGINT
#endif

typedef long pid_t;
typedef long gid_t;
typedef long uid_t;
typedef unsigned long u_long;
typedef unsigned int u_int;
typedef unsigned short u_short;
typedef unsigned char u_char;
typedef int     ssize_t;
typedef char *   caddr_t;        /* core address */
typedef	long	fd_mask;
#define	NBBY	8		/* number of bits in a byte */
#define	NFDBITS	(sizeof(fd_mask) * NBBY)	/* bits per mask */
#ifndef howmany
#define	howmany(x, y)	(((x) + ((y) - 1)) / (y))
#endif

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

struct ip_mreq {
        struct in_addr  imr_multiaddr;  /* IP multicast address of group */
        struct in_addr  imr_interface;  /* local IP address of interface */
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

#ifndef ETIME
#define ETIME 1
#endif

#ifndef SIGKILL
#define SIGKILL SIGTERM
#endif

#define fork() 0
#define setsid() {}


#ifndef startupSocket
#define startupSocket() {WSADATA wsaData;  \
  if (WSAStartup(0x0101, &wsaData))    \
    fprintf(stderr, "Could not start up WinSock.\n"); }
#endif

#ifndef FILE_SOCKET
#define FILE_SOCKET int
#endif

#ifndef fdopen_socket
#define fdopen_socket(f, g) &f
#endif

#ifndef fclose_socket
#define fclose_socket(f) closesocket(*f)
#endif

extern char getc_socket(FILE_SOCKET *f);
extern ssize_t write_socket(int fildes, const void *buf, size_t nbyte);
extern int sendmsg(int s, const struct msghdr *msg, int flags);

/* end of 'ifdef WIN32' */
#else
#error "Not Unix or WIN32 -- what system is this?"
#endif

#if !defined(sun4) && !defined(hp) && !defined(nextstep) && !defined(linux)
#include <sys/select.h>  /* select() */
#endif

#endif /* end of ifdef SYSDEP_H */
