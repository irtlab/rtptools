/*
* RTP translator.
*
* Usage:
*   rtptrans [host]/port[/ttl] [host]/port[/ttl] [...]
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
* Columbia University
*
*
* Additionally, the translator can translate VAT packets into RTP packets. 
* Thereby, the VAT control packets are translated into RTCP SDES packets with
* a CNAME and a NAME entry. However, this is only entended to be used in the 
* following configuration:
* VAT packets arriving on a multicast connection are translated into RTP and
* sent over a unicast link. RTP packets are not translated -not yet at least-
* into VAT packets and and all packets arriving on unicast links are not
* changed at all. Therefore, currently mainly the following topology is 
* supported:
*
*    multicast VAT -> translator -> unicast RTP
*
*    and on the way back it should lokk like this
*
*    multicast VAT <- translator <- unicast VAT
*
*
* This means that the audio agent on the unicast link should be able to use
* both VAT and RTP.
*
* Author: Dorgham Sisalem
* GMD Fokus, Berlin
*
* Bug fixes and supstantial improvements by 
* Stephen Casner <casner@precept.com>
* 
*/

#include <sys/types.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <netinet/in.h>  /* struct sockaddr_in */
#include <sys/time.h>    /* gettimeofday() */
#include <arpa/inet.h>   /* inet_ntoa() */
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>      /* select(), perror() */
#include <stdlib.h>      /* getopt(), atoi() */
#include <memory.h>      /* memset() */
#include "rtp.h"
#include "rtpdump.h"
#include "ansi.h"
#include "notify.h"
#include "multimer.h"
#include "vat.h"
#include "sysdep.h"

#define PAD(x,n) (((n) - ((x) & (n-1))) & (n-1))
#define MAX_HOST 10

static int debug = 0;
static int hostc = 0;
static int multi_sock[3] = {0,0,0};
static struct {
  int sock;
  struct sockaddr_in sin;
} side[MAX_HOST][3];  /* [host][proto] */

/* 
 * create a linked list for traversing the sequence numbers of the different 
 * streams arriving over a multicast link to a unicast network
 */
typedef struct stream_id{
  int seq;
  int addr;
  int next_ts;
  struct stream_id* next;
} STREAM;

typedef struct stream_id stream;

/* 
 * We need to keep in memory the sequence number of the last sent packet
 * for each data stream.
 */
static stream *head;   /* start of list */
static stream *middle; /* middle of the list, keeping this pointer reduces the
                        * avarage searching path to a 1/4 of the list instead of
                        * 1/2.
                        */
static stream *last;   /* Last seen stream, as the probability that the next
                        * packet will belong to the last seen one this reduces
                        * the searching path even further.
                        */
static int list_len;

int create_stream(int addr, int next)
{
  stream *elem;
  int i;
  stream* new_stream=( stream *) malloc(sizeof( stream));
  if(new_stream==NULL) {
    perror("can not create a new steam identifier ");
    exit (0);
  }
  /* init created stream */
  new_stream->next=NULL;
  new_stream->addr=addr;
  new_stream->seq=addr;
  new_stream->next_ts=next;
  if(head==NULL) { /* initialize the list */
    head=new_stream;
    middle=new_stream;
    list_len=1;
#if !defined(nextstep)
    srand48(rand());  /* initialize random number generator */
#else
    srand(rand());    /* (fred) This is surprising */
#endif
    new_stream->seq=rand();
    last=new_stream;
    return new_stream->seq;
  }
  new_stream->seq=rand(); /* init the first sequence number for this stream */
  if((list_len++)%2) {
    for(elem=head, i=1; i<(list_len/2)+(list_len%2);i++,elem=elem->next);
    middle=elem;
  }
  /* organize the list in order of the ssrc */
  if(head->addr>addr) {
    new_stream->next=head;
    head=new_stream;
    last=new_stream;
    return new_stream->seq;
  }

  for(elem=head;elem->next!=NULL;elem=elem->next) {
    if(elem->next->addr>addr) {
      new_stream->next=elem->next;
      elem->next=new_stream;
      last=new_stream;
      return new_stream->seq;
    } 
  }
      ;
  if(elem->next==NULL) {
    elem->next=new_stream;
    last=new_stream;
    return new_stream->seq;
  }   
  return new_stream->seq;
}


/* 
 * Add a stream to the list.
 */
int find_stream(int addr, int ts, int next, int m)
{
  stream *element;
  /* packet belongs to the last seen stream */
  if ((last!=NULL)&&(addr==last->addr)) {
     last->seq+=1;
     if (ts != last->next_ts && !m)
       last->seq+=1;   /* approximate missing some packets */
     last->next_ts = next;
     return(last->seq);
  }
  if((middle!=NULL)&&(addr>=middle->addr))
    element=middle;
  else
    element=head;
  for(; (element!=NULL)&&(element->addr<=addr); element=element->next) {
    if(element->addr==addr) {
      element->seq+=1;
       if (ts != element->next_ts && !m)
         element->seq += 1;  /* approximate missing some packets */
       element->next_ts = next;
      return (element->seq);
    }
  }
  return(create_stream(addr, next));
}

struct sdes_msg {
  rtcp_common_t header;
  struct rtcp_sdes sdes;
};

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
  const int VAT_LEN=8;
  vat_hdr_t *vat_hdr;
  rtp_hdr_t *rtp_hdr;
  rtp_hdr_t rtp_hdr_send;

  proto = ((int)client & 1);
  /* Read packet data from socket. */
  addr_len = sizeof(sin_from);
  len = recvfrom(sock, packet, sizeof(packet), 0,
        (struct sockaddr *)&sin_from, &addr_len);
  rtp_hdr=(rtp_hdr_t *)packet;
  if (debug) {
    struct timeval now;

    gettimeofday(&now, 0);
    printf("%0.3f %s %4d [%s/%d]\n", 
      now.tv_sec + now.tv_usec/1e6, 
      rtp_hdr->version==2 ? (proto ? "RTCP" : "RTP ") :
        rtp_hdr->version==0 ? (proto ? "vatC" : "vat ") : "UKWN", len,
      inet_ntoa(sin_from.sin_addr), ntohs(sin_from.sin_port));
  }

  /* do not translate packets that already use RTP or arrive over the unicast
   link*/
  if ((rtp_hdr->version==2)||((sock!=multi_sock[0])&&(sock!=multi_sock[1]))) {
    rtp_hdr=(rtp_hdr_t *)packet;
    for (i = 0; i < hostc; i++) {
      if (side[i][proto].sock != sock
          && side[i][proto].sin.sin_addr.s_addr != INADDR_ANY) {
        if (sendto(side[i][2].sock, packet, len, 0,
          (struct sockaddr *)&side[i][proto].sin,sizeof(side[i][proto].sin))<0)
//        perror("sendto RTCP");
          ;
      }
    }
  }
  else {
    struct msghdr msg;
    if (!proto) { /* translate VAT packets */
      struct iovec iov[2];
      char type; 
      int samples = len-VAT_LEN;
      vat_hdr=(vat_hdr_t *)packet;   

      if(vat_hdr->flags&VATHF_NEWTS)
        rtp_hdr_send.m = 1;
      else
        rtp_hdr_send.m = 0;
      type= vat_hdr->flags&VATHF_FMTMASK;

      switch (type) {

      case VAT_AUDF_GSM:
        samples = (samples/33)*160;

      case VAT_AUDF_MULAW8:
      case VAT_AUDF_G721:   /* samples not right for this */
      case VAT_AUDF_G723:   /* samples not right for this */
        rtp_hdr_send.pt=type;
        break;

      case VAT_AUDF_IDVI:
        samples = (samples-4)*2;
        rtp_hdr_send.pt=5;
        break;

      case VAT_AUDF_L16_16:
      case VAT_AUDF_L16_44:
      case VAT_AUDF_LPC4:
      case VAT_AUDF_CELP:
      case VAT_AUDF_LPC1:
      case VAT_AUDF_UNDEF :
        default:
        rtp_hdr_send.pt=115;   /* hopefully unused */
        perror(" unknown codecs ");
        break;
      }
      rtp_hdr_send.ssrc    = sin_from.sin_addr.s_addr;
      rtp_hdr_send.seq     = find_stream(rtp_hdr_send.ssrc, vat_hdr->ts,
         vat_hdr->ts + samples, rtp_hdr_send.m);
      rtp_hdr_send.version = RTP_VERSION;
      rtp_hdr_send.p       = 0;
      rtp_hdr_send.x       = 0;
      rtp_hdr_send.cc      = 0;
      rtp_hdr_send.ts      = vat_hdr->ts;

#if defined(Linux) || defined(WIN32)
      /* 
       * Stupid little Linux and stupid big Win32 does not support
       * sendmsg(), thus, use copying instead; contributed by Lutz
       * Grueneberg <gruen@rvs.uni-hannover.de>.
       */
      {
        unsigned char mbuf[10000];
        int mlength = 0;
        memcpy (&mbuf[mlength], (char *)&(rtp_hdr_send), sizeof(rtp_hdr_t)-4);
        mlength += sizeof(rtp_hdr_t)-4;
        memcpy (&mbuf[mlength], packet+VAT_LEN, len-VAT_LEN);
        mlength += len-VAT_LEN;
        for (i = 0; i < hostc; i++) {
          if (side[i][proto].sock != sock) {
            if (sendto(side[i][2].sock, mbuf, mlength, 0,
                        &(side[i][proto].sin), sizeof(side[i][proto].sin))!=
                mlength)
              perror("sendmsg RTCP");
          }
        }
      }
#else 
      iov[0].iov_base = (char *)&(rtp_hdr_send);
      iov[0].iov_len = sizeof(rtp_hdr_t)-4;
      iov[1].iov_base = packet+VAT_LEN;
      iov[1].iov_len = len-VAT_LEN;
      msg.msg_iov = iov;
      msg.msg_iovlen = 2;
      for (i = 0; i < hostc; i++) {
        if (side[i][proto].sock != sock) {
          msg.msg_name = (caddr_t ) &side[i][proto].sin;
          msg.msg_namelen = sizeof(side[i][proto].sin);
#if defined(__FreeBSD__) || defined(__linux__) || defined(__darwin__) /* Or presumably other BSD 4.4 systems */
          msg.msg_control = 0;
          msg.msg_controllen = 0;
#else
          msg.msg_accrights = 0;
          msg.msg_accrightslen = 0;
#endif
          if ((sendmsg(side[i][2].sock, &msg,0))!= 
            iov[0].iov_len +iov[1].iov_len)
            perror("sendmsg RTCP");
          }
       }
#endif /* Linux || WIN32 */
    }
    else if (((struct CtrlMsgHdr *)packet)->type == 1) /* vat ID messages */{
      rtcp_t *rtcp_msg;
      struct sdes_msg *ctl_msg;
      rtcp_sdes_item_t *item;
      int len=0;
      int length;
      item=NULL;

      /* total length of the packet = IP address+ site entry of vat+ 2 type+ 2 
        length + 4 common header+ 4 ssrc + 8 empty RR */
      length = strlen(inet_ntoa(sin_from.sin_addr)) +
        strlen(packet+sizeof(struct CtrlMsgHdr)) + 12 + 8;
      length = ((length/4)*4)+4;
      rtcp_msg=(rtcp_t *)malloc(length);
      memset(rtcp_msg,0,length);

      /* init RR */
      rtcp_msg->common.version=2;
      rtcp_msg->common.p=0;
      rtcp_msg->common.count=0;
      rtcp_msg->common.pt=201;
      rtcp_msg->r.rr.ssrc=sin_from.sin_addr.s_addr;
      rtcp_msg->common.length=(8 >> 2) - 1;

      ctl_msg=(struct sdes_msg *)&rtcp_msg->r.rr.rr[0];
      item=( rtcp_sdes_item_t *) ((char *)ctl_msg+8);
      /* init CNAME */
      item->type=1;
      strcpy(item->data,inet_ntoa(sin_from.sin_addr));
      item->length=strlen(item->data);
      len=item->length+2;

      item=(rtcp_sdes_item_t *)((char *)item +len);
      /* init NAME */
      strcpy(item->data,packet+sizeof(struct CtrlMsgHdr));
      item->length=strlen(item->data);
      item->type=2;
      len+=item->length+2;


      ctl_msg->header.version=2;
      ctl_msg->header.p=0;
      ctl_msg->header.count=1;
      ctl_msg->header.pt=202;
      ctl_msg->sdes.src=sin_from.sin_addr.s_addr;
      ctl_msg->header.length=((length-8) >> 2) - 1;

      for (i = 0; i < hostc; i++) {
        if (side[i][proto].sock != sock) {
          if (sendto(side[i][2].sock,(char *)rtcp_msg,
             ((rtcp_msg->common.length+1)+(ctl_msg->header.length+1))*4,
             0, (struct sockaddr *)&side[i][proto].sin,
             sizeof(side[i][proto].sin)) < 0)
          perror("sendto RTCP");
        }
      }
      free(rtcp_msg);
    }/* control messages */
  }
  return NOTIFY_DONE;
} /* socket_handler */


void usage(char *argv0)
{
  fprintf(stderr, 
"Usage: %s\
 host/port[/ttl]\
 host/port[/ttl] [...]\n",
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

  /* Set up socket. */
  startupSocket();
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

  if (argc - optind < 2) {
    fprintf(stderr, "%s: Requires two host/port[/tll].\n", argv[0]);
    usage(argv[0]);
    exit(1);
  }

  /* Parse host descriptions. */
  for (i = 0; i < argc - optind; i++) {
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
        usage(argv[0]);
        exit(1);
      }
      host[i].sin.sin_port = htons(port);
      s = strchr(s+1, '/');
      if (s) {
        host[i].ttl = atoi(s+1);
      }
    }
    host[i].sin.sin_addr = host2ip(host[i].name);
    if (host[i].sin.sin_addr.s_addr == -1) {
      fprintf(stderr, "%s: Invalid host. %s\n", argv[0], host[i].name);
      usage(argv[0]);
      exit(1);
    }
    if (IN_CLASSD(ntohl(host[i].sin.sin_addr.s_addr))) {
      host[i].mreq.imr_multiaddr        = host[i].sin.sin_addr;
      host[i].mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    }
    hostc++;
  }

  /* Create/bind sockets. */
  for (i = 0; i < hostc; i++) { /* hosts (unicast or multicast) */
    for (j = 0; j < 3; j++) { /* receive ports (RTP, RTCP), send */
      side[i][j].sock = socket(PF_INET, SOCK_DGRAM, 0);
      if (side[i][j].sock < 0) {
        perror("socket");
        exit(1);
      }
      if (setsockopt(side[i][j].sock, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse,
                   sizeof(reuse)) == -1)
        perror("setsockopt: reuseaddr");
      if (j < 2) {
        side[i][j].sin = host[i].sin;
        side[i][j].sin.sin_port = htons(ntohs(host[i].sin.sin_port) + j);
      }
      else {
        side[i][j].sin.sin_family = AF_INET;
        side[i][j].sin.sin_addr.s_addr = INADDR_ANY;
        side[i][j].sin.sin_port = 0;
      }

      /* Bind to multicast address. */
      sin = side[i][j].sin;
      if (IN_CLASSD(ntohl(host[i].sin.sin_addr.s_addr))) {
        if (!multi_sock[j]) {
          multi_sock[j]=side[i][j].sock;  /* save number of multicast socket */
        }
        if (j==2 && setsockopt(side[i][j].sock, IPPROTO_IP, IP_MULTICAST_TTL,
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
        if (j < 2 && setsockopt(side[i][j].sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
            (char *)&host[i].mreq, sizeof(host[i].mreq)) < 0) {
          perror("IP_ADD_MEMBERSHIP");
          exit(1);
        }
        if (j==2 && setsockopt(side[i][j].sock, IPPROTO_IP, IP_MULTICAST_LOOP,
            (char *)&loop, sizeof(loop)) < 0) {
          perror("IP_MULTICAST_LOOP");
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
      if (j < 2) {
        notify_set_input_func((Notify_client)j, socket_handler, 
          side[i][j].sock);
      }
    } /* for j (protocols) */
  } /* for i (hosts) */

  if ((c = notify_start()) != NOTIFY_OK) {
    fprintf(stderr, "%s: Notifier error %d.\n", argv[0], c);
    perror("select");
  };

  return 0;
} /* main */
