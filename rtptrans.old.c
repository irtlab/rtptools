/*
* RTP translator.
*
* Usage:
*   rtptrans host/port/ttl host/port/ttl [...]
*
* Forwards RTP/RTCP packets from one of the named sockets to all
* others.  Addresses can be a multicast or unicast.  TTL values for
* unicast addresses are ignored. (Actually, doesn't check whether
* packets are RTP or not.)
*
* It would be easy to add transcoding on a packet-by-packet basis (as
*   long as the sampling rate doesn't change).
*
* Author: Henning Schulzrinne
* GMD Fokus, Berlin
*
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>  /* struct sockaddr_in */
#include <arpa/inet.h>   /* inet_ntoa() */
#include <sys/time.h>    /* gettimeofday() */
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <values.h>
#include <unistd.h>      /* select(), perror() */
#include <stdlib.h>      /* getopt(), atoi() */
#include "rtp.h"
#include "rtpdump.h"
#include "ansi.h"
#include "notify.h"
#include "multimer.h"

static char rcsid[] = "$Id$";

#define MAX_HOST 10

static int debug = 0;
static int hostc = 0;
static struct {
  int sock;
  struct sockaddr_in sin;
} side[MAX_HOST][2];  /* [host][proto] */


/*
* Handle file input events from network sockets.
*/
static Notify_value socket_handler(Notify_client client, int sock)
{
  int len, addr_len;
  int proto;
  struct sockaddr_in sin_from;
  char packet[8192];
  int i;

  proto = ((int)client & 1);

  /* Read packet data from socket. */
  addr_len = sizeof(sin_from);
  len = recvfrom(sock, packet, sizeof(packet), 0,
        (struct sockaddr *)&sin_from, &addr_len);

  if (debug) {
    struct timeval now;

    gettimeofday(&now, 0);
    printf("%0.3f %s %4d [%s/%d]\n", 
      now.tv_sec + now.tv_usec/1e6, 
      proto ? "RTCP" : "RTP ", len,
      inet_ntoa(sin_from.sin_addr), ntohs(sin_from.sin_port));
  }
  for (i = 0; i < hostc; i++) {
    if (side[i][proto].sock != sock) {
      if (sendto(side[i][proto].sock, packet, len, 0,
        (struct sockaddr *)&side[i][proto].sin, sizeof(side[i][proto].sin)) < 0)
        perror("sendto RTCP");
    }
  }

  return NOTIFY_DONE;
} /* socket_handler */


void usage(char *argv0)
{
  fprintf(stderr, 
"Usage: %s \
[address]/port/ttl\
[address]/port/ttl [...]\n",
 argv0);
}

int main(int argc, char *argv[])
{
  int c;
  struct {
    char *name;
    unsigned char ttl;
    struct sockaddr_in sin;
    struct ip_mreq mreq;
  } host[MAX_HOST];
  struct sockaddr_in sin;   /* generic bind */
  extern int optind;
  char loop = 0;  /* multicast loop */
  int reuse = 1;  /* reuse address */
  int i, j;

  extern struct in_addr host2ip(char *);

  while ((c = getopt(argc, argv, "d?h")) != EOF) {
    switch(c) {
    case 'd':
      debug = 1;
      break;
    case '?':
    case 'h':
      usage(argv[0]);
      exit(1);
      break;
    }
  }

  if (optind == argc) {
    usage(argv[0]);
    exit(1);
  }

  /* Parse host descriptions. */
  for (i = 0; i < argc-optind; i++) {
    char *s;

    if (i >= MAX_HOST) break;
    host[i].ttl  = 16;
    host[i].name = argv[optind+i];
    host[i].sin.sin_family = AF_INET;
    s = strchr(host[i].name, '/');
    if (!s) {
      usage(argv[0]);
      exit(1);
    }
    else {
      int port;

      *s = '\0';
      port = atoi(s+1);
      if (port & 1) {
        fprintf(stderr, "%s: Port must be even.\n", argv[0]);
        /* exit(1); */
      }
      host[i].sin.sin_port = htons(port);
      s = strchr(s+1, '/');
      if (s) {
        host[i].ttl = atoi(s+1);
      }
    }
    host[i].sin.sin_addr = host2ip(host[i].name);
    if (IN_CLASSD(host[i].sin.sin_addr.s_addr)) {
      host[i].mreq.imr_multiaddr        = host[i].sin.sin_addr;
      host[i].mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    }
    hostc++;
  }

  /* Create/bind sockets. */
  for (i = 0; i < hostc; i++) { /* hosts (unicast or multicast) */
    for (j = 0; j < 2; j++) { /* ports (RTP, RTCP) */
      side[i][j].sock = socket(PF_INET, SOCK_DGRAM, 0);
      if (side[i][j].sock < 0) {
        perror("socket");
        exit(1);
      }
      if (setsockopt(side[i][j].sock, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse,
                   sizeof(reuse)) == -1)
        perror("setsockopt: reuseaddr");
      side[i][j].sin = host[i].sin;
      side[i][j].sin.sin_port = htons(ntohs(host[i].sin.sin_port) + j);

      /* Bind to multicast address. */
      sin = side[i][j].sin;
      if (IN_CLASSD(host[i].sin.sin_addr.s_addr)) {
        if (setsockopt(side[i][j].sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
            (char *)&host[i].mreq, sizeof(host[i].mreq)) < 0) {
            perror("IP_ADD_MEMBERSHIP");
            exit(1);
        }
        if (setsockopt(side[i][j].sock, IPPROTO_IP, IP_MULTICAST_TTL,
            (char *)&host[i].ttl, sizeof(host[i].ttl)) < 0) {
          perror("IP_MULTICAST_TTL");
          exit(1);
        }
again:
        if (bind(side[i][j].sock, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
          if (errno == EADDRNOTAVAIL) {
            sin.sin_addr.s_addr = INADDR_ANY;
            goto again;
          }
          else {
            perror("bind multicast");
            exit(1);
          }
        }
        if (setsockopt(side[i][j].sock, IPPROTO_IP, IP_MULTICAST_LOOP,
            (char *)&loop, sizeof(loop)) < 0) {
          perror("IP_MULTICAST_LOOP");
          exit(1);
        }
      } /* multicast */
      /* unicast */
      else {
        sin.sin_addr.s_addr = INADDR_ANY;
        if (bind(side[i][j].sock, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
          perror("bind unicast");
          exit(1);
        }
      }
      notify_set_input_func((Notify_client)j, socket_handler, 
        side[i][j].sock);
    } /* for j (protocols) */
  } /* for i (hosts) */

  if ((c = notify_start()) != NOTIFY_OK) {
    fprintf(stderr, "%s: Notifier error %d.\n", argv[0], c);
    perror("select");
  };

  return 0;
} /* main */
