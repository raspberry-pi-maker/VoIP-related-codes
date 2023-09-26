#include "global.h"
#include "util.h"
/*
in the sdp find the line that contains the token (copies from the token)
*/
int getline(const char *pSDP, const char *szTok, char *pLine)
{
	const char *pStart = strstr(pSDP, szTok);
	if(NULL == pStart) return -1;
	while(*pStart != '\r' && *pStart != '\n'){
		*pLine = *pStart;
		pLine++;
		pStart++;
	}
	*pLine = 0x00;
	return 0;
}
const char *ip_to_str(const u_char *ip, char *pstr)
{
  sprintf(pstr, "%d.%d.%d.%d", (int)ip[0], (int)ip[1], (int)ip[2], (int)ip[3]);
  return(const char *)pstr;
}

// this function converts pcmu (8bit) to pcm(16bit)
int16_t ulaw2linear(u_char u_val)
{
  int16_t t;
   /* Complement to obtain normal u-law value. */
   u_val = ~u_val;
   
   /*
    * Extract and bias the quantization bits. Then
    * shift up by the segment number and subtract out the bias.
    */
   t = ((u_val & QUANT_MASK) << 3) + BIAS;
   t <<= ((unsigned)u_val & SEG_MASK) >> SEG_SHIFT;
   
   return ((u_val & SIGN_BIT) ? (BIAS - t) : (t - BIAS));  
}

// this function converts pcma (8bit) to pcm(16bit)
int16_t alaw2linear(u_char	a_val)
{
   int16_t t;
   int16_t seg;
   
   a_val ^= 0x55;
   
   t = (a_val & QUANT_MASK) << 4;
   seg = ((unsigned)a_val & SEG_MASK) >> SEG_SHIFT;
   switch (seg) {
   case 0:
      t += 8;
      break;
   case 1:
      t += 0x108;
      break;
   default:
      t += 0x108;
      t <<= seg - 1;
   }
   return ((a_val & SIGN_BIT) ? t : -t);
}


char* dummy_get_raw_pcm (const char *p, int *bytes_read)
{
    long lSize;
    char *pcm_buf;
    size_t result;
    FILE *fp_pcm;

    fp_pcm = fopen (p, "rb");
    if (fp_pcm == NULL) {
        fprintf(stderr, "pcm File[%s] open error", p);
        return NULL;
    }

    // obtain file size:
    fseek (fp_pcm , 0 , SEEK_END);
    lSize = ftell (fp_pcm);
    rewind (fp_pcm);

    // allocate memory to contain the whole file:
    pcm_buf = (char*) malloc (sizeof(char) * lSize);
    if (pcm_buf == NULL) {
        fprintf(stderr, "pcm File[%s] memory allocation error", p);
        fclose(fp_pcm);
        return NULL;
    }

    // copy the file into the pcm_buf:
    result = fread (pcm_buf, 1, lSize, fp_pcm);
    if (result != lSize) {
        fprintf(stderr, "pcm File[%s] reading error", p);
        fclose(fp_pcm);
        free(pcm_buf);
        return NULL;
    }

    *bytes_read = (int) lSize;
    return pcm_buf;
}
