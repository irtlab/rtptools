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

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>

#ifndef WIN32
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#endif

#include "sysdep.h"

int host2ip(char*, struct in_addr*);
int hpt(char*, struct sockaddr_in*, unsigned char*);

/*
* Parse an IP address or a host name.
* For NULL or an empty 'host', set to INADDR_ANY.
* Return 0 for success, -1 in error.
*/
int
host2ip(char *host, struct in_addr *in)
{
	in_addr_t a;
	struct hostent *hep;

	if (host == NULL || *host == '\0') {
		in->s_addr = INADDR_ANY;
		return 0;
	}
	if ((a = inet_addr(host)) != INADDR_NONE) {
		/* a valid dotted-decimal */
		in->s_addr = a;
		return 0;
	}
	if ((hep = gethostbyname(host))) {
		/* a resolved hostname */
		memcpy(in, hep->h_addr, sizeof(struct in_addr));
		return 0;
	}
	return -1;
}

/* Parse [host]/port[/ttl]. Return 0 if ok, -1 on error;
 * fill in sockaddr; set ttl if requested. */
int 
hpt(char *h, struct sockaddr_in * sin, unsigned char *ttl)
{
	char *p = NULL, *t = NULL;
	int port;

	sin->sin_family = AF_INET;
	if (NULL == (p = strchr(h, '/')))
		return -1;
	*p++ = '\0';
	if (host2ip(h, &sin->sin_addr) == -1) {
		warnx("Cannot parse address: %s", h);
		return -1;
	}
	if ((t = strchr(p, '/')))
		*t++ = '\0';
	port = atoi(p);
	if (port <= 0) {
		warnx("RTP port number must be positive, not %d", port);
		return -1;
	}
	if (port & 1) {
		warnx("RTP port number should be even, not %d", port);
		return -1;
	}
	sin->sin_port = htons(port);
	if (t && ttl)
		*ttl = atoi(t);
	return 0;
}
