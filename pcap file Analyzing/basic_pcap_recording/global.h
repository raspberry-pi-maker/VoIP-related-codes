#pragma once
#include <stdio.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>
#include <pcap.h>
#include <string.h>
#include <iostream>
#include <list>
#include <string>
#include <time.h>
#include <unistd.h>

using namespace std;

typedef struct _RTP_HEADER
{
  #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__  
    unsigned char cc:4;          /* CSRC count */
    unsigned char extension:1;   /* header extension flag */
    unsigned char padding:1;     /* padding flag */
    unsigned char version:2;     /* protocol version */

    unsigned char type:7;          /* payload type */
    unsigned char marker:1;      /* marker bit */
    uint16_t seq:16;            /* sequence number */
  #elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    unsigned char version:2;     /* protocol version */
    unsigned char padding:1;     /* padding flag */
    unsigned char extension:1;   /* header extension flag */
    unsigned char cc:4;          /* CSRC count */
    unsigned char marker:1;      /* marker bit */
    unsigned char type:7;          /* payload type */
    uint16_t seq:16;            /* sequence number */
  #else
    #error Byte Order Error
  #endif    
    uint32_t ts;                /* timestamp */
    uint32_t ssrc;              /* synchronization source */
} RTP_HEADER;

// WAVE file header format
typedef struct {
    unsigned char   chunk_id[4];        // RIFF string
    unsigned int    chunk_size;         // overall size of file in bytes (36 + data_size)
    unsigned char   sub_chunk1_id[8];   // WAVEfmt string with trailing null char
    unsigned int    sub_chunk1_size;    // 16 for PCM.  This is the size of the rest of the Subchunk which follows this number.
    unsigned short  audio_format;       // format type. 1-PCM, 3- IEEE float, 6 - 8bit A law, 7 - 8bit mu law
    unsigned short  num_channels;       // Mono = 1, Stereo = 2
    unsigned int    sample_rate;        // 8000, 16000, 44100, etc. (blocks per second)
    unsigned int    byte_rate;          // SampleRate * NumChannels * BitsPerSample/8
    unsigned short  block_align;        // NumChannels * BitsPerSample/8
    unsigned short  bits_per_sample;    // bits per sample, 8- 8bits, 16- 16 bits etc
    unsigned char   sub_chunk2_id[4];   // Contains the letters "data"
    unsigned int    sub_chunk2_size;    // NumSamples * NumChannels * BitsPerSample/8 - size of the next chunk that will be read
} wav_header_t;

typedef struct
{
    //basic info
    u_char my_ip[4];
    u_char other_ip[4];    //other party
    uint32_t ssrc;  
    string filename;
    FILE *fh;
} CALL;

#define RTP_HEADER_OFFSET 12

const int QUANT_MASK = 0xf;  /* Quantization field mask. */
const int NSEGS = 8;         /* Number of A-law segments. */
const int SEG_SHIFT = 4;     /* Left shift for segment number. */
const int SEG_MASK = 0x70;   /* Segment field mask. */
const int SIGN_BIT = 0x80;	/* Sign bit for a A-law byte. */
const int G711_SAMPLES_PER_FRAME = 160;
const int TABLE_SIZE = 8;
const int BIAS = 0x84;		/* Bias for linear code. */
const int CLIP = 8159;


inline uint32_t get_ssrc(RTP_HEADER *ph){ return ntohl(ph->ssrc);}
inline uint32_t get_timestamp(RTP_HEADER *ph){ return ntohl(ph->ts);}
inline uint16_t get_seq(RTP_HEADER *ph){ return  ntohs(ph->seq);}
inline int get_payload(RTP_HEADER *ph) { return ph->type;}
inline int get_version(RTP_HEADER *ph) { return ph->version;}
const char *ip_to_str(const u_char *ip, char *pstr);

//util.cpp functions
CALL *add_call(const u_char *myip, const u_char *otherip, uint32_t ssrc);
CALL *findcall(const u_char *myip, const u_char *otherip, uint32_t ssrc);
int add_rtp(CALL *call, const u_char *rtp, int size, int payload);
void finalize_calls();

extern list<CALL *> g_calls;
extern char g_outputdir[];


