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

#ifdef WIN32
#include <fcntl.h>       /* O_BINARY */
#else
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <err.h>
#endif

#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "rtp.h"
#include "vat.h"
#include "payload.h"
#include "rtpdump.h"
#include "sysdep.h"

#define RTPFILE_VERSION "1.0"

extern int hpt(char*, struct sockaddr_in*, unsigned char*);
extern struct pt payload[];

typedef uint32_t member_t;

static int verbose = 0; /* decode */

typedef enum {
	F_invalid,
	F_dump,
	F_header,
	F_hex,
	F_rtcp,
	F_short,
	F_payload,
	F_ascii
} t_format;


int dyn_payload_rtpevent = 101; /* Default payload type for RTPEvent packets */

static void usage(const char *argv0)
{
  fprintf(stderr, "usage: %s "
	"[-F hex|ascii|rtcp|short|payload|dump|header] "
	"[-f infile] [-o outfile] [-t minutes] [-x bytes] "
	"[-d event_payload_id] [address]/port > file\n", argv0);
}


static void done(int sig)
{
  exit(0);
}

/*
* Convert timeval 'a' to double.
*/
static double tdbl(struct timeval *a)
{
  return a->tv_sec + a->tv_usec/1e6;
}


/*
* Print buffer 'buf' of length 'len' bytes in hex format to file 'out'.
*/
static void hex(FILE *out, char *buf, int len)
{
  int i;

  for (i = 0; i < len; i++) {
    fprintf(out, "%02x", (unsigned char)buf[i]);
  }
} /* hex */


/*
* Open network sockets.
*/
static int open_network(char *host, int data, int sock[], struct
  sockaddr_in *sin)
{
  struct ip_mreq mreq;      /* multicast group */
  int i;
  int nfds = 0;
 
  /* multicast */
  if (host) {
    mreq.imr_multiaddr = sin->sin_addr;
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    sin->sin_addr = mreq.imr_multiaddr;
  }
  /* unicast */
  else {
    mreq.imr_multiaddr.s_addr = INADDR_ANY;
    sin->sin_addr.s_addr      = INADDR_ANY;
  }

  /* create/bind sockets */
  for (i = 0; i < 2; i++) {
    int one = 1;

    /* open data socket only if necessary */
    sock[i] = -1;
    if (!data && i == 0) continue;

    sock[i] = socket(PF_INET, SOCK_DGRAM, 0);
    if (sock[i] < 0) {
      perror("socket");
      exit(1);
    }
    if (sock[i] > nfds) nfds = sock[i];

    if (IN_CLASSD(ntohl(mreq.imr_multiaddr.s_addr))) {
      if (setsockopt(sock[i], SOL_SOCKET, SO_REUSEADDR, (char *) &one,
                   sizeof(one)) == -1)
        perror("setsockopt: reuseaddr");
    }

#ifdef SO_REUSEPORT /* BSD 4.4 */
    if (setsockopt(sock[i], SOL_SOCKET, SO_REUSEPORT, (char *) &one,
           sizeof(one)) == -1)
      perror("setsockopt: reuseport");
#endif

    sin->sin_port = htons(ntohs(sin->sin_port) + i);
    if (bind(sock[i], (struct sockaddr *)sin, sizeof(*sin)) < 0) {
      if (errno == EADDRNOTAVAIL) {
        sin->sin_addr.s_addr = INADDR_ANY;
        if (bind(sock[i], (struct sockaddr *)sin, sizeof(*sin)) < 0) {
          perror("bind");
          exit(1);
        }
      }
      else {
        perror("bind");
        exit(1);
      }
    }

    if (IN_CLASSD(ntohl(mreq.imr_multiaddr.s_addr))) {
      if (setsockopt(sock[i], IPPROTO_IP,
        IP_ADD_MEMBERSHIP, (char *)&mreq, sizeof(mreq)) < 0) {
          perror("IP_ADD_MEMBERSHIP");
          exit(1);
      }
    }
  }
  return nfds;
} /* open_network */


/*
* Write a header to the current output file.
* The header consists of an identifying string, followed
* by a binary structure.
*/
static void rtpdump_header(FILE *out, struct sockaddr_in *sin,
  struct timeval *start)
{
  RD_hdr_t hdr;

  fprintf(out, "#!rtpplay%s %s/%d\n", RTPFILE_VERSION,
    inet_ntoa(sin->sin_addr), ntohs(sin->sin_port));
  hdr.start.tv_sec  = htonl(start->tv_sec);
  hdr.start.tv_usec = htonl(start->tv_usec);
  hdr.source = sin->sin_addr.s_addr;
  hdr.port   = sin->sin_port;
  hdr.padding = 0; /* value will be compiler dependent unless clear it */
  if (fwrite((char *)&hdr, sizeof(hdr), 1, out) < 1) {
    perror("fwrite");
    exit(1);
  }
} /* rtpdump_header */


/*
* Return type of packet, either "RTP", "RTCP", "VATD" or "VATC".
*/
static const char *parse_type(int type, char *buf)
{
  if (type == 0) {
    rtp_hdr_t *r = (rtp_hdr_t *)buf;
    return r->version == RTP_VERSION ? "RTP" : "VATD";
  }
  else {
    rtcp_t *r = (rtcp_t *)buf;
    return r->common.version == RTP_VERSION ? "RTCP" : "VATC";
  }
} /* parse_type */


/*
* Return header length of RTP packet contained in 'buf'.
*/
static int parse_header(char *buf)
{
  rtp_hdr_t *r = (rtp_hdr_t *)buf;
  rtp_hdr_ext_t *ext;
  int ext_len;
  int hlen = 0;

  if (r->version == 0) {
    vat_hdr_t *v = (vat_hdr_t *)buf;
    hlen = 8 + v->nsid * 4;
  }
  else if (r->version == RTP_VERSION) {
    hlen = 12 + r->cc * 4;
    if (r->x) {  /* header extension */
      ext = (rtp_hdr_ext_t *)((char *)buf + hlen);
      ext_len = ntohs(ext->len);
      hlen += 4 + (ext_len * 4);
    }
  }

  return hlen;
} /* parse_header */


/*
* Return header length.
*/
static int parse_data(FILE *out, char *buf, int len)
{
  rtp_hdr_t *r = (rtp_hdr_t *)buf;
  rtp_hdr_ext_t *ext;
  int i, ext_len;
  int hlen = 0;
  char *payload_data = NULL;

  /* Show vat format packets. */
  if (r->version == 0) {
    vat_hdr_t *v = (vat_hdr_t *)buf;
    fprintf(out, "nsid=%d flags=0x%x confid=%u ts=%u\n",
      v->nsid, v->flags, v->confid, v->ts);
    hlen = 8 + v->nsid * 4;
  }
  else if (r->version == RTP_VERSION) {
    hlen = 12 + r->cc * 4;
    if (len < hlen) {
      fprintf(out, "RTP header too short (%d bytes for %d CSRCs).\n",
         len, r->cc);
      return hlen;
    }
    fprintf(out,
    "v=%d p=%d x=%d cc=%d m=%d pt=%d (%s,%d,%d) seq=%u ts=%lu ssrc=0x%lx ",
      r->version, r->p, r->x, r->cc, r->m,
      r->pt, payload[r->pt].enc, payload[r->pt].ch, payload[r->pt].rate,
      ntohs(r->seq),
      (unsigned long)ntohl(r->ts),
      (unsigned long)ntohl(r->ssrc));
    for (i = 0; i < r->cc; i++) {
      fprintf(out, "csrc[%d]=0x%0lx ", i, (long)r->csrc[i]);
    }

    /* Check if it is an RTPEvent Packet */
    if (dyn_payload_rtpevent == r->pt)
    {

	    payload_data = (char*)buf + 12 + (r->cc*4);
	    if (payload_data[0] == 10)
	    {

		    payload_data[0] = '*';
	    }
	    else if (payload_data[0] == 11)
	    {
		    payload_data[0] = '#';
	    }
	    else
	    {
		    payload_data[0] += '0';
	    }
      	    fprintf(out, "dtmf=%c duration=%dms end=%d ", payload_data[0], ntohs(*(short*)(payload_data+2))/8,(payload_data[1]>>7)&0x1);
	    
    }

    if (r->x) {  /* header extension */
      ext = (rtp_hdr_ext_t *)((char *)buf + hlen);
      ext_len = ntohs(ext->len);

      fprintf(out, "ext_type=0x%x ", ntohs(ext->ext_type));
      fprintf(out, "ext_len=%d ", ext_len);

      if (ext_len) {
        fprintf(out, "ext_data=");
        hex(out, (char *)(ext+1), (ext_len*4));
        fprintf(out, " ");
      }
    }
  }
  else {
    fprintf(out, "RTP version wrong (%d).\n", r->version);
  }
  return hlen;
} /* parse_data */


/*
* Print minimal per-packet information: time, timestamp, sequence number.
*/
static void parse_short(FILE *out, struct timeval now, char *buf, int len)
{
  rtp_hdr_t *r = (rtp_hdr_t *)buf;

  if (r->version == 0) {
    vat_hdr_t *v = (vat_hdr_t *)buf;
    fprintf(out, "%ld.%06ld %lu\n",
      (v->flags ? -now.tv_sec : now.tv_sec), (long)now.tv_usec,
      (unsigned long)ntohl(v->ts));
  }
  else if (r->version == 2) {
    fprintf(out, "%ld.%06ld %lu %u\n",
      (r->m ? -now.tv_sec : now.tv_sec), (long)now.tv_usec,
      (unsigned long)ntohl(r->ts), ntohs(r->seq));
  }
  else {
    fprintf(out, "RTP version wrong (%d).\n", r->version);
  }
} /* parse_short */


/*
* Show SDES information for one member.
*/
static void member_sdes(FILE *out, member_t m, rtcp_sdes_type_t t, char *b, int len)
{
  static struct {
    rtcp_sdes_type_t t;
    const char *name;
  } map[] = {
    {RTCP_SDES_END,    "end"},
    {RTCP_SDES_CNAME,  "CNAME"},
    {RTCP_SDES_NAME,   "NAME"},
    {RTCP_SDES_EMAIL,  "EMAIL"},
    {RTCP_SDES_PHONE,  "PHONE"},
    {RTCP_SDES_LOC,    "LOC"},
    {RTCP_SDES_TOOL,   "TOOL"},
    {RTCP_SDES_NOTE,   "NOTE"},
    {RTCP_SDES_PRIV,   "PRIV"},
    {11,               "SOURCE"},
    {0,0}
  };
  int i;
  char num[10];

  sprintf(num, "%d", t);
  for (i = 0; map[i].name; i++) {
    if (map[i].t == t) break;
  }
  fprintf(out, "%s=\"%*.*s\" ",
    map[i].name ? map[i].name : num, len, len, b);
} /* member_sdes */


/*
* Parse one SDES chunk (one SRC description). Total length is 'len'.
* Return new buffer position or zero if error.
*/
static char *rtp_read_sdes(FILE *out, char *b, int len)
{
  rtcp_sdes_item_t *rsp;
  uint32_t src = *(uint32_t *)b;
  int total_len = 0;

  len -= 4;  /* subtract SSRC from available bytes */
  if (len <= 0) {
    return 0;
  }
  rsp = (rtcp_sdes_item_t *)(b + 4);
  for (; rsp->type; rsp = (rtcp_sdes_item_t *)((char *)rsp + rsp->length + 2)) {
    member_sdes(out, src, rsp->type, rsp->data, rsp->length);
    total_len += rsp->length + 2;
  }
  if (total_len >= len) {
    fprintf(stderr,
      "Remaining length of %d bytes for SSRC item too short (has %u bytes)\n",
      len, total_len);
    return 0;
  }
  b = (char *)rsp + 1;
  /* skip padding */
  return b + ((4 - ((intptr_t)b & 0x3)) & 0x3);
} /* rtp_read_sdes */


/*
* Return length parsed, -1 on error.
*/
static int parse_control(FILE *out, char *buf, int len)
{
  rtcp_t *r;         /* RTCP header */
  int i;

  r = (rtcp_t *)buf;
  /* Backwards compatibility: VAT header. */
  if (r->common.version == 0) {
    struct CtrlMsgHdr *v = (struct CtrlMsgHdr *)buf;

    fprintf(out, "flags=0x%x type=0x%x confid=%u\n",
      v->flags, v->type, v->confid);
  }
  else if (r->common.version == RTP_VERSION) {
    fprintf(out, "\n");
    while (len > 0) {
      len -= (ntohs(r->common.length) + 1) << 2;
      if (len < 0) {
        /* something wrong with packet format */
        fprintf(out, "Illegal RTCP packet length %d words.\n",
           ntohs(r->common.length));
        return -1;
      }

      switch (r->common.pt) {
      case RTCP_SR:
        fprintf(out, " (SR ssrc=0x%lx p=%d count=%d len=%d\n",
          (unsigned long)ntohl(r->r.rr.ssrc),
          r->common.p, r->common.count,
          ntohs(r->common.length));
        fprintf(out, "  ntp=%lu.%lu ts=%lu psent=%lu osent=%lu\n",
          (unsigned long)ntohl(r->r.sr.ntp_sec),
          (unsigned long)ntohl(r->r.sr.ntp_frac),
          (unsigned long)ntohl(r->r.sr.rtp_ts),
          (unsigned long)ntohl(r->r.sr.psent),
          (unsigned long)ntohl(r->r.sr.osent));
        for (i = 0; i < r->common.count; i++) {
          fprintf(out, "  (ssrc=0x%lx fraction=%g lost=%ld last_seq=%lu jit=%lu lsr=%lu dlsr=%lu )\n",
           (unsigned long)ntohl(r->r.sr.rr[i].ssrc),
           r->r.sr.rr[i].fraction / 256.,
           (long)RTCP_GET_LOST(&r->r.sr.rr[i]),
           (unsigned long)ntohl(r->r.sr.rr[i].last_seq),
           (unsigned long)ntohl(r->r.sr.rr[i].jitter),
           (unsigned long)ntohl(r->r.sr.rr[i].lsr),
           (unsigned long)ntohl(r->r.sr.rr[i].dlsr));
        }
        fprintf(out, " )\n");
        break;

      case RTCP_RR:
        fprintf(out, " (RR ssrc=0x%lx p=%d count=%d len=%d\n",
          (unsigned long)ntohl(r->r.rr.ssrc), r->common.p, r->common.count,
          ntohs(r->common.length));
        for (i = 0; i < r->common.count; i++) {
          fprintf(out, "  (ssrc=0x%lx fraction=%g lost=%ld last_seq=%lu jit=%lu lsr=%lu dlsr=%lu )\n",
            (unsigned long)ntohl(r->r.rr.rr[i].ssrc),
            r->r.rr.rr[i].fraction / 256.,
            (long)RTCP_GET_LOST(&r->r.rr.rr[i]),
            (unsigned long)ntohl(r->r.rr.rr[i].last_seq),
            (unsigned long)ntohl(r->r.rr.rr[i].jitter),
            (unsigned long)ntohl(r->r.rr.rr[i].lsr),
            (unsigned long)ntohl(r->r.rr.rr[i].dlsr));
        }
        fprintf(out, " )\n");
        break;

      case RTCP_SDES:
        fprintf(out, " (SDES p=%d count=%d len=%d\n",
          r->common.p, r->common.count, ntohs(r->common.length));
        buf = (char *)&r->r.sdes;
        for (i = 0; i < r->common.count; i++) {
          int remaining = (ntohs(r->common.length) << 2) -
                          (buf - (char *)&r->r.sdes);

          fprintf(out, "  (src=0x%lx ",
            (unsigned long)ntohl(((struct rtcp_sdes *)buf)->src));
          if (remaining > 0) {
            buf = rtp_read_sdes(out, buf,
              (ntohs(r->common.length) << 2) - (buf - (char *)&r->r.sdes));
            if (!buf) return -1;
          }
          else {
            fprintf(stderr, "Missing at least %d bytes.\n", -remaining);
            return -1;
          }
          fprintf(out, ")\n");
        }
        fprintf(out, " )\n");
        break;

      case RTCP_BYE:
        fprintf(out, " (BYE p=%d count=%d len=%d\n",
          r->common.p, r->common.count, ntohs(r->common.length));
        for (i = 0; i < r->common.count; i++) {
          fprintf(out, "  (ssrc[%d]=0x%0lx ", i,
            (unsigned long)ntohl(r->r.bye.src[i]));
        }
        fprintf(out, ")\n");
        if (ntohs(r->common.length) > r->common.count) {
          buf = (char *)&r->r.bye.src[r->common.count];
          fprintf(out, "reason=\"%*.*s\"", *buf, *buf, buf+1);
        }
        fprintf(out, " )\n");
        break;

      /* invalid type */
      default:
        fprintf(out, "(? pt=%d src=0x%lx)\n", r->common.pt,
          (unsigned long)ntohl(r->r.sdes.src));
      break;
      }
      r = (rtcp_t *)((uint32_t *)r + ntohs(r->common.length) + 1);
    }
  }
  else {
    fprintf(out, "invalid version %d\n", r->common.version);
  }
  return len;
} /* parse_control */


/*
* Process one packet and write it to file 'out' using format 'format'.
*/
static void packet_handler(FILE *out, t_format format, int trunc,
  double dstart, struct timeval now, int ctrl,
  struct sockaddr_in sin, int len, RD_buffer_t *packet)
{
  double dnow = tdbl(&now);
  int hlen;   /* header length */
  int offset;
  switch(format) {
    case F_header:
      offset = (dnow - dstart) * 1000;
      packet->p.hdr.offset = htonl(offset);
      packet->p.hdr.plen   = ctrl ? 0 : htons(len);
      /* leave only header */
      if (ctrl == 0) len = parse_header(packet->p.data);
      packet->p.hdr.length = htons(len + sizeof(packet->p.hdr));
      if (fwrite((char *)&packet, len + sizeof(packet->p.hdr), 1, out) == 0) {
        perror("fwrite");
        exit(1);
      }
      break;

    case F_dump:
      hlen = ctrl ? len : parse_header(packet->p.data);
      offset = (dnow - dstart) * 1000;
      packet->p.hdr.offset = htonl(offset);
      packet->p.hdr.plen   = ctrl ? 0 : htons(len);
      /* truncation of payload */
      if (!ctrl && (len - hlen > trunc)) len = hlen + trunc;
      packet->p.hdr.length = htons(len + sizeof(packet->p.hdr));
      if (fwrite((char *)packet, len + sizeof(packet->p.hdr), 1, out) == 0) {
        perror("fwrite");
        exit(1);
      }
      break;

    case F_payload:
      if (ctrl == 0) {
        hlen = parse_header(packet->p.data);
        if (fwrite(packet->p.data + hlen, len - hlen, 1, out) == 0) {
          perror("fwrite");
          exit(1);
        }
      }
      break;

    case F_short:
      if (ctrl == 0) parse_short(out, now, packet->p.data, len);
      break;

    case F_hex:
    case F_ascii:
      if (ctrl == 0) {
        fprintf(out, "%ld.%06ld %s len=%d from=%s:%u ",
                now.tv_sec, (long)now.tv_usec, parse_type(ctrl, packet->p.data),
                len, inet_ntoa(sin.sin_addr), ntohs(sin.sin_port));
        parse_data(out, packet->p.data, len);
        if (format == F_hex) {
          hlen = parse_header(packet->p.data);
          fprintf(out, "data=");
          hex(out, packet->p.data + hlen, trunc < len ? trunc : len - hlen);
        }
        fprintf(out, "\n");
      }
    case F_rtcp:
      if (ctrl == 1) {
        fprintf(out, "%ld.%06ld %s len=%d from=%s:%u ",
                now.tv_sec, (long)now.tv_usec, parse_type(ctrl, packet->p.data),
                len, inet_ntoa(sin.sin_addr), ntohs(sin.sin_port));
        parse_control(out, packet->p.data, len);
      }
      break;

    case F_invalid:
      break;
  }
} /* packet_handler */


int main(int argc, char *argv[])
{
  int c;
  static struct {
    const char *name;
    t_format format;
  } formats[] = {
    {"dump",    F_dump},
    {"header",  F_header},
    {"hex",     F_hex},
    {"rtcp",    F_rtcp},
    {"short",   F_short},
    {"payload", F_payload},
    {"ascii",   F_ascii},
    {0,0}
  };
  t_format format = F_ascii;
  struct sockaddr_in sin, rtp;
  struct timeval start;
  struct timeval timeout;   /* timeout to limit recording */
  double dstart;            /* time as double */
  float duration = 1000000; /* maximum duration in seconds */
  int trunc    = 1000000;   /* bytes to show for F_hex and F_dump */
  enum {FromFile, FromNetwork} source;
  int sock[2];
  FILE *in = stdin;         /* input file to use instead of sockets */
  FILE *out = stdout;       /* output file */
  fd_set readfds;
  extern char *optarg;
  extern int optind;
  int i;
  int nfds = 0;
  extern double tdbl(struct timeval *);

  startupSocket();
  while ((c = getopt(argc, argv, "F:f:o:t:x:d:h")) != EOF) {
    switch(c) {
    /* output format */
    case 'F':
      format = F_invalid;
      for (i = 0; formats[i].name; i++) {
        if (strncasecmp(formats[i].name, optarg, strlen(optarg)) == 0) {
          format = formats[i].format;
          break;
        }
      }
      if (format == F_invalid) {
        usage(argv[0]);
        exit(1);
      }
      break;

    /* input file (instead of network connection) */
    case 'f':
      if (!(in = fopen(optarg, "rb"))) {
        perror(optarg);
        exit(1);
      }
      break;

    /* output file */
    case 'o':
      if (!(out = fopen(optarg, "wb"))) {
        perror(optarg);
        exit(1);
      }
      break;

    /* recording duration in minutes or fractions thereof */
    case 't':
      duration = atof(optarg) * 60;
      break;

    /* bytes to show for F_hex or F_dump */
    case 'x':
      if (0 == (trunc = atoi(optarg))) {
	  warnx("Invalid -x value");
	  usage(argv[0]);
	  exit(1);
      }
      break;

      /* Dynamic Payload Type for RTPEVENT packets */
    case 'd':
      dyn_payload_rtpevent = atoi(optarg);
      break;

    case '?':
    case 'h':
      usage(argv[0]);
      exit(1);
      break;
    }
  }

#if defined(WIN32)
  /* On Windows, make sure stdout and stdin use the binary format
   * if using F_dump or F_header. */
  if (format == F_dump || format == F_header) {
    if (out == stdout) {
      setmode(fileno(stdout), O_BINARY);
    }
  }
  if (in == stdin) {
    setmode(fileno(stdin), O_BINARY);
  }
#endif

  /* set maximum time to gather packets */
  timeout.tv_sec  = duration;
  timeout.tv_usec = (duration - (float)timeout.tv_sec) * 1000000.0;

  /* if no optional arguments, we are reading from a file */
  if (optind == argc) {
    source = FromFile;
    sock[0] = fileno(in);  /* stdin */
    sock[1] = -1;          /* not used */
    memset(&sin, 0, sizeof(struct sockaddr_in));
    RD_header(in, &sin, &start, 0);
    dstart = 0.;
  }
  else {
    source = FromNetwork;
    if (hpt(argv[optind], &sin, NULL) == -1) {
      usage(argv[0]);
      exit(1);
    }
    rtp = sin;
    nfds = open_network(argv[optind], format != F_rtcp, sock, &sin);
    gettimeofday(&start, 0);
    dstart = tdbl(&start);
  }

  /* write header for dump file */
  if (format == F_dump || format == F_header)
    rtpdump_header(out, &rtp, &start);

  /* signal handler */
  signal(SIGINT, done);
  signal(SIGTERM, done);
  signal(SIGHUP, done);

  /* main loop */
  while (1) {
    int len;
    RD_buffer_t packet;
    struct timeval now;
    double dnow;

    if (source == FromNetwork) {
      FD_ZERO(&readfds);
      if (sock[0] >= 0) FD_SET(sock[0], &readfds);
      if (sock[1] >= 0) FD_SET(sock[1], &readfds);
      c = select(nfds+1, &readfds, 0, 0, &timeout);
      if (c < 0) {
        perror("select");
        exit(1);
      }
      /* end of recording time reached */
      else if (c == 0) {
        if (verbose)
          fprintf(stderr, "Time limit reached.\n");
        exit(0);
      }
      for (i = 0; i < 2; i++) {
        if (sock[i] >= 0 && FD_ISSET(sock[i], &readfds)) {
          socklen_t alen = sizeof(sin);

          /* subtract elapsed time from remaining timeout */
          gettimeofday(&now, 0);
          dnow = tdbl(&now);
          timeout.tv_sec = duration - (dnow - dstart);
          if (timeout.tv_sec < 0) timeout.tv_sec = 0;

          len = recvfrom(sock[i], packet.p.data, sizeof(packet.p.data),
            0, (struct sockaddr *)&sin, &alen);
          packet_handler(out, format, trunc, dstart, now, i, sin, len, &packet);
        }
      }
    }
    else {
      len = RD_read(in, &packet);
      if (len == 0) exit(0);
      now.tv_sec = packet.p.hdr.offset / 1000.;
      now.tv_usec = (packet.p.hdr.offset % 1000) * 1000;
      dnow = tdbl(&now);
      /* plen>0: data =0: control */
      i = (packet.p.hdr.plen == 0);
      /* arbitrary, obviously invalid value */
      sin.sin_addr.s_addr = INADDR_ANY; sin.sin_port = 0;
      packet_handler(out, format, trunc, dstart, now, i, sin, len, &packet);
    }
  }
  return 0;
} /* main */
