#include <sys/types.h>
#include <sys/socket.h>      /* struct sockaddr */
#include <netdb.h>           /* gethostbyname() */
#include <netinet/in.h>      /* sockaddr_in */
#include <arpa/inet.h>       /* inet_addr() */
#include <rpcsvc/ypclnt.h>   /* YP */
#include <string.h>          /* strlen() added by Akira 12/27/01 */ 
#include "sysdep.h"

static char rcsid[] = "$Id$";

/*
* Return IP address given host name 'host'.
* If 'host' is "", set to INADDR_ANY.
*/
struct in_addr host2ip(char *host)
{
  struct in_addr in;
  register struct hostent *hep;

  /* Check whether this is a dotted decimal. */
  if (!host || *host == '\0') {
    in.s_addr = INADDR_ANY;
  }
  else if ((in.s_addr = inet_addr(host)) != -1) {
  }
  /* Attempt to resolve host name via DNS. */
  else if ((hep = gethostbyname(host))) {
    in = *(struct in_addr *)(hep->h_addr_list[0]);
  }
  /* As a last resort, try YP. */
  else {
    static char *domain = 0;  /* YP domain */
    char *value;              /* key value */
    int value_len;            /* length of returned value */

    if (!domain) yp_get_default_domain(&domain);
    if (yp_match(domain, "hosts.byname", host, strlen(host), &value, &value_len) == 0) {
      in.s_addr = inet_addr(value);
    }
  }
  return in;
} /* host2ip */
