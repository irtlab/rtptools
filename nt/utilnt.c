#include <time.h>
#include <sys/timeb.h>

#include "sysdep.h"

int gettimeofday(struct timeval *tp, void *t) {
  struct _timeb tv;

  _ftime(&tv);
  tp->tv_sec = tv.time;
  tp->tv_usec = tv.millitm*1000;

  return 1;
} 

ssize_t write_socket(int fildes, const void *buf, size_t nbyte) {
	return send(fildes, buf, nbyte, 0);
}

int sendmsg(int s, const struct msghdr *msg, int flags) {
}

