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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#ifndef WIN32
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

#if HAVE_HSEARCH
#include <search.h>
#else
#include "compat-hsearch.h"
#endif

#include "sysdep.h"
#include "notify.h"
#include "rtp.h"
#include "rtpdump.h"
#include "multimer.h"
#include "payload.h"

#define READAHEAD 16 /* must be power of 2 */

extern int hpt(char*, struct sockaddr_in*, unsigned char*);
extern struct pt payload[];

static int verbose = 0;        /* be chatty about packets sent */
static int wallclock = 0;      /* use wallclock time rather than timestamps */
static uint32_t begin = 0;      /* time of first packet to send */
static uint32_t end = UINT32_MAX; /* when to stop sending */
static FILE *in;               /* input file */
static int sock[2];            /* output sockets */
static int first = -1;         /* time offset of first packet */
static RD_buffer_t buffer[READAHEAD];

struct rtts {
	struct timeval	rt; /* real time */
	unsigned long	ts; /* timestamp */
};

struct ssrc {
	uint32_t	ssrc;
	struct rtts	rtts;
	struct ssrc*	next;
} *list = NULL;

static struct ssrc*
find(uint32_t ssrc)
{
	struct ssrc* this = NULL;
	for (this = list; this; this = this->next)
		if (ssrc == this->ssrc)
			return this;
	return NULL;
}

static void
insert(struct ssrc* ssrc)
{
	if (ssrc == NULL)
		return;
	if (list)
		ssrc->next = list;
	else
		list = ssrc;
}

static void usage(char *argv0)
{
  fprintf(stderr, "usage: %s "
	"[-hTv] [-b begin] [-e end] [-f file] [-s port] "
	"address/port[/ttl]\n", argv0);
  exit(1);
} /* usage */


static double tdbl(struct timeval *a)
{
  return a->tv_sec + a->tv_usec/1e6;
} /* tdbl */


/*
* Transmit RTP/RTCP packet on output socket and mark as read.
*/
static void play_transmit(int b)
{
  if (b >= 0 && buffer[b].p.hdr.length) {
    if (send(sock[buffer[b].p.hdr.plen == 0],
        buffer[b].p.data, buffer[b].p.hdr.length, 0) < 0) {
      perror("write");
    }

    buffer[b].p.hdr.length = 0;
  }
} /* play_transmit */


/*
* Timer handler: read next record from file and insert into timer
* handler.
*/
static Notify_value play_handler(Notify_client client)
{
  static struct timeval start;  /* generation time of first played back p. */
  struct timeval now;           /* current time */
  struct timeval next;          /* next packet generation time */
  struct ssrc* ssrc = NULL;
  struct rtts t;
  uint32_t ts  = 0;
  uint8_t  pt  = 0;
  rtp_hdr_t *r;
  int b = (int)client;  /* buffer to be played now */
  int rp;        /* read pointer */

  gettimeofday(&now, 0);

  /* playback scheduled packet */
  play_transmit(b);

  /* If we are done, skip rest. */
  if (end == 0) return NOTIFY_DONE;

  if (verbose > 0 && b >= 0) {
    printf("! %1.3f %s(%3d;%3d) t=%6lu",
      tdbl(&now), buffer[b].p.hdr.plen ? "RTP " : "RTCP",
      buffer[b].p.hdr.length, buffer[b].p.hdr.plen,
      (unsigned long)buffer[b].p.hdr.offset);

    if (buffer[b].p.hdr.plen) {
      r = (rtp_hdr_t *)buffer[b].p.data;
      printf(" pt=%u ssrc=%8lx %cts=%9lu seq=%5u",
        (unsigned int)r->pt,
        (unsigned long)ntohl(r->ssrc), r->m ? '*' : ' ',
        (unsigned long)ntohl(r->ts), ntohs(r->seq));
    }
    printf("\n");
  }

  /* Find available buffer. */
  for (rp = 0; rp < READAHEAD; rp++) {
    if (!buffer[rp].p.hdr.length) break;
  }

  /* Get next packet; try again if we haven't reached the begin time. */
  do {
    if (RD_read(in, &buffer[rp]) == 0) return NOTIFY_DONE;
  } while (buffer[rp].p.hdr.offset < begin);

  /*
   * If new packet is after end of alloted time, don't insert into list
   * and set 'end' to zero to avoid reading any more packets from
   * file.
   */
  if (buffer[rp].p.hdr.offset > end) {
    buffer[rp].p.hdr.length = 0; /* erase again */
    end = 0;
    return NOTIFY_DONE;
  }

  r = (rtp_hdr_t *)buffer[rp].p.data;

  /* Remember wallclock and recording time of first valid packet. */
  if (first < 0) {
    start = now;
    first = buffer[rp].p.hdr.offset;
  }
  buffer[rp].p.hdr.offset -= first;

  if (buffer[rp].p.hdr.plen && r->version == 2 && !wallclock) {
    ts  = ntohl(r->ts);
    pt  = r->pt;
    if ((ssrc = find(ntohl(r->ssrc)))) {
    /* found in the list of sources: compute playout instant */
	double d;
	t = ssrc->rtts;
	d = payload[pt].rate ? ((1.0)*(int)(ts - t.ts)) / payload[pt].rate : 0;
	next.tv_sec  = t.rt.tv_sec  + (int)d;
	next.tv_usec = t.rt.tv_usec + (d - (int)d) * 1000000;
	if (verbose) {
	  printf(". %1.3f t=%6lu pt=%u ts=%lu,%lu rp=%2d b=%d d=%f\n",
		tdbl(&next),
		(unsigned long)buffer[rp].p.hdr.offset, (unsigned int)r->pt,
		(unsigned long)ts, (unsigned long)t.ts, rp, b, d);
	}

    /*
      next.tv_sec  = t->rt.tv_sec  + (int)d;
      next.tv_usec = t->rt.tv_usec + (d - (int)d) * 1000000;
	*/

    } else {
	/* not on source list: insert and play based on wallclock. */
	next.tv_sec  = start.tv_sec  +  buffer[rp].p.hdr.offset / 1000;
	next.tv_usec = start.tv_usec + (buffer[rp].p.hdr.offset % 1000) * 1000;
	ssrc = calloc(1, sizeof(struct ssrc));
	ssrc->ssrc = ntohl(r->ssrc);
	insert(ssrc);
    }
  }
  else {
  /* RTCP or vat or playing back by wallclock: compute next playout time */
    next.tv_sec  = start.tv_sec  + buffer[rp].p.hdr.offset/1000;
    next.tv_usec = start.tv_usec + (buffer[rp].p.hdr.offset%1000) * 1000;
  }

  if (next.tv_usec >= 1000000) {
    next.tv_usec -= 1000000;
    next.tv_sec  += 1;
  }

  if (ssrc) {
    ssrc->rtts.rt = next;
    ssrc->rtts.ts = ts;
  }

  timer_set(&next, play_handler, (Notify_client)rp, 0);
  return NOTIFY_DONE;
} /* play_handler */



int main(int argc, char *argv[])
{
  unsigned char ttl = 1;
  static struct sockaddr_in sin;
  static struct sockaddr_in from;
  int sourceport = 0;  /* source port */
  int on = 1;          /* flag */
  int i;
  int c;
  extern char *optarg;
  extern int optind;

  /* For NT, we need to start the socket; dummy function otherwise */
  startupSocket();

  in = stdin; /* Changed below if -f specified */

  /* parse command line arguments */
  while ((c = getopt(argc, argv, "b:e:f:p:Ts:vh")) != EOF) {
    switch(c) {
    case 'b':
      begin = atof(optarg) * 1000;
      break;
    case 'e':
      end = atof(optarg) * 1000;
      break;
    case 'f':
      if (!(in = fopen(optarg, "rb"))) {
        perror(optarg);
        exit(1);
      }
      break;
    case 'T':
      wallclock = 1;
      break;
    case 's':  /* locked source port */
      sourceport = atoi(optarg);
      break;
    case 'v':
      verbose = 1;
      break;
    case '?':
    case 'h':
      usage(argv[0]);
      break;
    }
  }

//  ftell(in);

  if (optind < argc) {
    if (hpt(argv[optind], &sin, &ttl) == -1) {
      fprintf(stderr, "%s: Invalid host. %s\n", argv[0], argv[optind]);
      usage(argv[0]);
      exit(1);
    }
    if (sin.sin_addr.s_addr == INADDR_ANY) {
      struct hostent *host;
      struct in_addr *local;
      if ((host = gethostbyname("localhost")) == NULL) {
        perror("gethostbyname()");
        exit(1);
      }
      local = (struct in_addr *)host->h_addr_list[0];
      sin.sin_addr = *local;
    }
  }

  /* read header of input file */
  if (RD_header(in, &sin, verbose) < 0) {
    fprintf(stderr, "Invalid header\n");
    exit(1);
  }

  /* create/connect sockets if they don't exist already */
  if (!sock[0]) {
    for (i = 0; i < 2; i++) {
      sock[i] = socket(PF_INET, SOCK_DGRAM, 0);
      if (sock[i] < 0) {
        perror("socket");
        exit(1);
      }
      sin.sin_port = htons(ntohs(sin.sin_port) + i);

      if (sourceport) {
        memset((char *)(&from), 0, sizeof(struct sockaddr_in));
        from.sin_family      = PF_INET;
        from.sin_addr.s_addr = INADDR_ANY;
        from.sin_port        = htons(sourceport + i);

        if (setsockopt(sock[i], SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
          perror("SO_REUSEADDR");
          exit(1);
        }

  #ifdef SO_REUSEPORT
        if (setsockopt(sock[i], SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on)) < 0) {
          perror("SO_REUSEPORT");
          exit(1);
        }
  #endif

        if (bind(sock[i], (struct sockaddr *)&from, sizeof(from)) < 0) {
          perror("bind");
          exit(1);
        }
      }

      if (connect(sock[i], (struct sockaddr *)&sin, sizeof(sin)) < 0) {
        perror("connect");
        exit(1);
      }

      if (IN_CLASSD(ntohl(sin.sin_addr.s_addr)) &&
          (setsockopt(sock[i], IPPROTO_IP, IP_MULTICAST_TTL, &ttl,
                   sizeof(ttl)) < 0)) {
        perror("IP_MULTICAST_TTL");
        exit(1);
      }
    }
  }

  /* initialize event queue */
  first = -1;
  for (i = 0; i < READAHEAD; i++) play_handler(-1);
  notify_start();

  return 0;
} /* main */
