/*
 * (c) 1998-2017 by Columbia University; all rights reserved
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

#include <sys/types.h>
#include <sys/socket.h>      /* struct sockaddr */
#include <netdb.h>           /* gethostbyname() */
#include <netinet/in.h>      /* sockaddr_in */
#include <arpa/inet.h>       /* inet_addr() */
#ifdef HAVE_YP
#include <rpcsvc/ypclnt.h>   /* YP */
#endif
#include <string.h>          /* strlen() added by Akira 12/27/01 */ 
#include "sysdep.h"

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
#ifdef HAVE_YP
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
#endif
  return in;
} /* host2ip */
