/*
 * rtp.h  --  RTP header file (RFC 1890)
 */
#include "types.h"   /* changed from <sys/types.h> by Akira 12/27/01 */
#include "sysdep.h"

#if 0 /* types.h has a better definition for this. by Akira 12/27/01 */
/*
 * The type definitions below are valid for 32-bit architectures and
 * may have to be adjusted for 16- or 64-bit architectures.
 */
typedef unsigned char  u_int8;
typedef unsigned short u_int16;
typedef unsigned int   u_int32;
typedef          short int16;
#endif

/*
 * System endianness -- determined by autoconf.
 */
#ifdef WORDS_BIGENDIAN
#define RTP_BIG_ENDIAN 1
#else
#define RTP_LITTLE_ENDIAN 1
#endif

/*
 * Current protocol version.
 */
#define RTP_VERSION    2

#define RTP_SEQ_MOD (1<<16)
#define RTP_MAX_SDES 255      /* maximum text length for SDES */

typedef enum {
    RTCP_SR   = 200,
    RTCP_RR   = 201,
    RTCP_SDES = 202,
    RTCP_BYE  = 203,
    RTCP_APP  = 204
} rtcp_type_t;

typedef enum {
    RTCP_SDES_END   = 0,
    RTCP_SDES_CNAME = 1,
    RTCP_SDES_NAME  = 2,
    RTCP_SDES_EMAIL = 3,
    RTCP_SDES_PHONE = 4,
    RTCP_SDES_LOC   = 5,
    RTCP_SDES_TOOL  = 6,
    RTCP_SDES_NOTE  = 7,
    RTCP_SDES_PRIV  = 8
} rtcp_sdes_type_t;

/*
 * RTP data header
 */
typedef struct {
#if RTP_BIG_ENDIAN
    unsigned int version:2;   /* protocol version */
    unsigned int p:1;         /* padding flag */
    unsigned int x:1;         /* header extension flag */
    unsigned int cc:4;        /* CSRC count */
    unsigned int m:1;         /* marker bit */
    unsigned int pt:7;        /* payload type */
#elif RTP_LITTLE_ENDIAN
    unsigned int cc:4;        /* CSRC count */
    unsigned int x:1;         /* header extension flag */
    unsigned int p:1;         /* padding flag */
    unsigned int version:2;   /* protocol version */
    unsigned int pt:7;        /* payload type */
    unsigned int m:1;         /* marker bit */
#else
#error Define one of RTP_LITTLE_ENDIAN or RTP_BIG_ENDIAN
#endif
    unsigned int seq:16;      /* sequence number */
    u_int32 ts;               /* timestamp */
    u_int32 ssrc;             /* synchronization source */
    u_int32 csrc[1];          /* optional CSRC list */
} rtp_hdr_t;

/* RTP Header Extension 
 */
typedef struct {
    u_int16 ext_type;         /* defined by profile */
    u_int16 len;              /* extension length in 32-bit word */
} rtp_hdr_ext_t;

/*
 * RTCP common header word
 */
typedef struct {
#if RTP_BIG_ENDIAN
    unsigned int version:2;   /* protocol version */
    unsigned int p:1;         /* padding flag */
    unsigned int count:5;     /* varies by packet type */
#elif RTP_LITTLE_ENDIAN
    unsigned int count:5;     /* varies by packet type */
    unsigned int p:1;         /* padding flag */
    unsigned int version:2;   /* protocol version */
#else
#error Define one of RTP_LITTLE_ENDIAN or RTP_BIG_ENDIAN
#endif
    unsigned int pt:8;        /* RTCP packet type */
    unsigned int length:16;   /* pkt len in words, w/o this word */
} rtcp_common_t;

/*
 * Big-endian mask for version, padding bit and packet type pair
 * XXX?
 */
#define RTCP_VALID_MASK (0xc000 | 0x2000 | 0xfe)
#define RTCP_VALID_VALUE ((RTP_VERSION << 14) | RTCP_SR)

/*
 * Reception report block
 */
typedef struct {
    u_int32 ssrc;             /* data source being reported */
    unsigned int fraction:8;  /* fraction lost since last SR/RR */
    int lost:24;              /* cumul. no. pkts lost (signed!) */
    u_int32 last_seq;         /* extended last seq. no. received */
    u_int32 jitter;           /* interarrival jitter */
    u_int32 lsr;              /* last SR packet from this source */
    u_int32 dlsr;             /* delay since last SR packet */
} rtcp_rr_t;

/*
 * SDES item
 */
typedef struct {
    u_int8 type;              /* type of item (rtcp_sdes_type_t) */
    u_int8 length;            /* length of item (in octets) */
    char data[1];             /* text, not null-terminated */
} rtcp_sdes_item_t;

/*
 * One RTCP packet
 */
typedef struct {
    rtcp_common_t common;     /* common header */
    union {
        /* sender report (SR) */
        struct {
            u_int32 ssrc;     /* sender generating this report */
            u_int32 ntp_sec;  /* NTP timestamp */
            u_int32 ntp_frac;
            u_int32 rtp_ts;   /* RTP timestamp */
            u_int32 psent;    /* packets sent */
            u_int32 osent;    /* octets sent */ 
            rtcp_rr_t rr[1];  /* variable-length list */
        } sr;

        /* reception report (RR) */
        struct {
            u_int32 ssrc;     /* receiver generating this report */
            rtcp_rr_t rr[1];  /* variable-length list */
        } rr;

        /* source description (SDES) */
        struct rtcp_sdes {
            u_int32 src;      /* first SSRC/CSRC */
            rtcp_sdes_item_t item[1]; /* list of SDES items */
        } sdes;

        /* BYE */
        struct {
            u_int32 src[1];   /* list of sources */
            /* can't express trailing text for reason */
        } bye;
    } r;
} rtcp_t;

typedef struct rtcp_sdes rtcp_sdes_t;
