#pragma once

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

int getline(const char *pSDP, const char *szTok, char *pLine);
const char *ip_to_str(const u_char *ip, char *pstr);
int16_t ulaw2linear(u_char u_val);
int16_t alaw2linear(u_char	a_val);

