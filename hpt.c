/*
* Parse [host]/port[/ttl].  Return 0 if ok, -1 if error; set sockaddr and
* ttl value.
*/
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "sysdep.h"

extern struct in_addr host2ip(char *);

int hpt(char *h, struct sockaddr *sa, unsigned char *ttl)
{
  char *s;
  struct sockaddr_in *sin = (struct sockaddr_in *)sa;

  sin->sin_family = AF_INET;

  /* first */
  s = strchr(h, '/');
  if (!s) {
    return -1;
  }
  else {
    int port;

    *s = '\0';
    port = atoi(s+1);
    sin->sin_port = htons(port);
    if (port & 1) {
      return -2;
    }
    s = strchr(s+1, '/');
    if (s && ttl) {
      *ttl = atoi(s+1);
    }
    sin->sin_addr = host2ip(h);
  }
  return 0;
} /* hpt */
