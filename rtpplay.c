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

#include <stdint.h>
#include <sys/types.h>
#include <sys/time.h>    /* gettimeofday() */
#include <sys/socket.h>  /* struct sockaddr */
#include <netinet/in.h>
#include <arpa/inet.h>   /* inet_ntoa() */
#include <netdb.h>       /* gethostbyname() */
#include <time.h>
#include <stdio.h>       /* stderr, printf() */
#include <string.h>
#include <stdlib.h>      /* perror() */
#include <unistd.h>      /* write() */
#include "sysdep.h"
#if HAVE_SEARCH_H
#include <search.h>      /* hash table */
#else
#include "hsearch.h"
#endif
#include "notify.h"      /* notify_start(), ... */
#include "rtp.h"         /* RTP headers */
#include "rtpdump.h"     /* RD_packet_t */
#include "multimer.h"    /* timer_set() */
#include "ansi.h"

#define READAHEAD 16 /* must be power of 2 */

static int verbose = 0;        /* be chatty about packets sent */
static int wallclock = 0;      /* use wallclock time rather than timestamps */
static uint32_t begin = 0;      /* time of first packet to send */
static uint32_t end = UINT32_MAX; /* when to stop sending */ 
static FILE *in;               /* input file */
static int sock[2];            /* output sockets */
static int first = -1;         /* time offset of first packet */
static RD_buffer_t buffer[READAHEAD];

static double period[128] = {  /* ms per timestamp difference */
  1/8000.,   /*  0: PCMU */
  1/8000.,   /*  1: 1016 */
  1/8000.,   /*  2: G721 */
  1/8000.,   /*  3: GSM  */
  1/8000.,   /*  4: G723 */
  1/8000.,   /*  5: DVI4 */
  1/16000.,  /*  6: DVI4 */
  1/8000.,   /*  7: LPC  */
  1/8000.,   /*  8: PCMA */
  1/16000.,  /*  9: G722 */
  1/44100.,  /* 10: L16  */
  1/44100.,  /* 11: L16  */
  0,         /* 12:      */
  0,         /* 13:      */
  1/90000.,  /* 14: MPA  */
  1/90000.,  /* 15: G728 */
  1/11025.,  /* 16: DVI4 */
  1/22050.,  /* 17: DVI4 */
  0,         /* 18:      */
  0,         /* 19:      */
  0,         /* 20:      */
  0,         /* 21:      */
  0,         /* 22:      */
  0,         /* 23:      */
  0,         /* 24:      */
  1/90000.,  /* 25: CelB */
  1/90000.,  /* 26: JPEG */  
  1/90000.,  /* 27:      */  
  1/90000.,  /* 28: nv   */  
  1/90000.,  /* 29:      */  
  1/90000.,  /* 30: */  
  1/90000.,  /* 31: H261 */  
  1/90000.,  /* 32: MPV  */  
  1/90000.,  /* 33: MP2T */  
  1/90000.,  /* 34: H263 */
};


static void usage(char *argv0)
{
  fprintf(stderr, "usage: %s "
	"[-hTv] [-b begin] [-e end] [-f file] [-p profile] [-s port] "
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
  ENTRY *e;                     /* hash table entry */
  struct rt_ts {
    struct timeval rt;          /* real-time */
    unsigned long ts;           /* timestamp */
  };
  struct rt_ts *t = 0;
  char ssrc[12];
  uint32_t ts  = 0;
  uint8_t  pt  = 0;
  uint16_t seq = 0;
  uint8_t  m   = 0;
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

  /* RTP played according to timestamp. */
  if (buffer[rp].p.hdr.plen && r->version == 2 && !wallclock) {
    ENTRY item;

    ts  = ntohl(r->ts);
    seq = ntohs(r->seq);
    pt  = r->pt;
    m   = r->m;
    sprintf(ssrc, "%lx", (unsigned long)ntohl(r->ssrc));

    /* find hash entry */
    item.key  = ssrc;
    item.data = 0;
    e = hsearch(item, FIND);

    /* If found in list of sources, compute playout instant. */
    if (e) {
      double d;

      t = (struct rt_ts *)e->data;
      d = period[pt] * (int)(ts - t->ts);
      next.tv_sec  = t->rt.tv_sec  + (int)d;
      next.tv_usec = t->rt.tv_usec + (d - (int)d) * 1000000;
      if (verbose)
        printf(". %1.3f t=%6lu pt=%u ts=%lu,%lu rp=%2d b=%d d=%f\n", tdbl(&next), 
        (unsigned long)buffer[rp].p.hdr.offset, (unsigned int)r->pt,
        (unsigned long)ts, (unsigned long)t->ts,
        rp, b, d);
    } else { /* If not on source list, insert and play based on wallclock. */
      item.key  = malloc(strlen(ssrc)+1);
      strcpy(item.key, ssrc);
      t = (struct rt_ts *)malloc(sizeof(struct rt_ts));
      item.data = (void *)t;
      next.tv_sec  = start.tv_sec  + buffer[rp].p.hdr.offset/1000;
      next.tv_usec = start.tv_usec + (buffer[rp].p.hdr.offset%1000) * 1000;
      e = hsearch(item, ENTER);
    }
  }
  /* RTCP or vat or playing back by wallclock. */
  else {
    /* compute next playout time */
    next.tv_sec  = start.tv_sec  + buffer[rp].p.hdr.offset/1000;
    next.tv_usec = start.tv_usec + (buffer[rp].p.hdr.offset%1000) * 1000;
    ssrc[0] = '\0';
  }

  if (next.tv_usec >= 1000000) {
    next.tv_usec -= 1000000;
    next.tv_sec  += 1;
  }

  /* Save correct value in record (for timestamp-based playback). */
  if (t) {
    t->rt = next;
    t->ts = ts; 
  }

  timer_set(&next, play_handler, (Notify_client)rp, 0);
  return NOTIFY_DONE;
} /* play_handler */


/*
* Read profile file, containing one line for each PT.
*/
static void profile(char *fn)
{
  FILE *f;
  int pt, r;

  if (!(f = fopen(fn, "r"))) {
    perror(fn);
    exit(1);
  }
  while (fscanf(f, "%d %d", &pt, &r) != EOF) {
    if (pt >= 0 && pt < 128 && r > 0 && r < 100000) {
      period[pt] = 1/(double)r;
    }
    else {
      fprintf(stderr, "PT=%d or rate=%d is invalid.\n", pt, r);
    }
  }
} /* profile */


int main(int argc, char *argv[])
{
  char ttl = 1;
  static struct sockaddr_in sin;
  static struct sockaddr_in from;
  int sourceport = 0;  /* source port */
  int on = 1;          /* flag */
  int i;
  int c;
  extern char *optarg;
  extern int optind;
  extern int hpt(char *h, struct sockaddr *sa, unsigned char *ttl);

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
    case 'p':
      profile(optarg);
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
    if (hpt(argv[optind], (struct sockaddr *)&sin, &ttl) < 0) {
      usage(argv[0]);
      exit(1);
    }
    if (sin.sin_addr.s_addr == -1) {
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
  hcreate(100); /* create hash table for SSRC entries */
  for (i = 0; i < READAHEAD; i++) play_handler(-1);
  notify_start();
  hdestroy();

  return 0;
} /* main */
