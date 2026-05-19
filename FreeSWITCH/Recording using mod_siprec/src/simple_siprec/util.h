#pragma once
#include <string_view>
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <string>
#include <string.h>
#include <regex>
#include <sys/stat.h>
#include "global.h"

#define RTP_HEADER_OFFSET 12

#define safe_fclose(fp)        \
    do {                      \
        if ((fp) != nullptr) {\
            fclose(fp);       \
            (fp) = nullptr;   \
        }                     \
    } while (0)

#define safe_close(fd)        \
    do {                      \
        if ((fd) != 0) {\
            close(fd);       \
        }                     \
    } while (0)
      
using namespace std;

const int QUANT_MASK = 0xf;  /* Quantization field mask. */
const int NSEGS = 8;         /* Number of A-law segments. */
const int SEG_SHIFT = 4;     /* Left shift for segment number. */
const int SEG_MASK = 0x70;   /* Segment field mask. */
const int SIGN_BIT = 0x80;	/* Sign bit for a A-law byte. */
const int G711_SAMPLES_PER_FRAME = 160;
const int TABLE_SIZE = 8;
const int BIAS = 0x84;		/* Bias for linear code. */
const int CLIP = 8159;

inline int get_payload(RTP_HEADER *ph) { return ph->type;}
inline uint16_t get_seq(RTP_HEADER *ph){ return ntohs(ph->seq);}


string ltrim(const string &s);
string rtrim(const string &s);
string trim(const string &s);

int getline(const char *pSDP, const char *szTok, char *pLine);

const char *ip_to_str(const u_char *ip, char *pstr);
int16_t ulaw2linear(u_char u_val);
int16_t alaw2linear(u_char	a_val);
int16_t pcm_mix(int a, int b);

unsigned char linear2ulaw(short pcm_val);
unsigned char linear2alaw(short pcm_val);
int16_t pcm_mix(int a, int b);
string exec_command(const char *cmd);

//sleep helper
void sleep_ns(long nsecs);
void sleep_ms(int msecs);
void sleep_s(int secs);

bool lock_pidfile(const char *name);

void get_exe_path(char *buffer, int buf_size);
bool file_exist(const char *filename);

