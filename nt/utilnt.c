#include <stdio.h>      /* fprintf(), stderr */
#include <time.h>
#include <sys/timeb.h>  /* ftime */
#include "sysdep.h"

/*
 * For
 *   1)startup winsock
 *   2)winfd_dummy for select()
 * by Akira 12/27/01
 */
int winfd_dummy = -1;       /* for select */

void startupSocket(void)
{
  WSADATA wsaData;      /* WSAStartup() */

  /* startup winsock */
  if (WSAStartup(MAKEWORD(1, 1), &wsaData)) {
    fprintf(stderr, "WSAStartup(): Could not start up WinSock.\n");
    exit(1);
  }

  /* create dummy fd for select() */
  winfd_dummy = socket(AF_INET, SOCK_DGRAM, 0);
  if (winfd_dummy == INVALID_SOCKET) {
    fprintf(stderr, "WSASocket: %d\n", WSAGetLastError ());
    exit(1);
  }

}

#if 0
/* obsolete code */
int gettimeofday(struct timeval *tp, void *dummy) {
  struct timeb tv;

  ftime(&tv);
  tp->tv_sec = tv.time;
  tp->tv_usec = tv.millitm*1000;

  return 0;
}
#endif

ssize_t write_socket(int fildes, const void *buf, size_t nbyte) {
  return send(fildes, buf, nbyte, 0);
}
