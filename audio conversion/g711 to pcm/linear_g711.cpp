/*
This program converts 8k, 16bit pcm file to g.711 format(8k, 8bit)
*/

#include <stdio.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <iostream>
#include <list>
#include <string>

using namespace std;

#define	SIGN_BIT	(0x80)		/* Sign bit for a A-law byte. */
#define	QUANT_MASK	(0xf)		/* Quantization field mask. */
#define	NSEGS		(8)		/* Number of A-law segments. */
#define	SEG_SHIFT	(4)		/* Left shift for segment number. */
#define	SEG_MASK	(0x70)		/* Segment field mask. */

#define	BIAS		(0x84)		/* Bias for linear code. */
#define CLIP            8159

static short seg_aend[8] = {0x1F, 0x3F, 0x7F, 0xFF,
			    0x1FF, 0x3FF, 0x7FF, 0xFFF};
static short seg_uend[8] = {0x3F, 0x7F, 0xFF, 0x1FF,
			    0x3FF, 0x7FF, 0xFFF, 0x1FFF};

void usage();
void do_process();
string input(""), output("");
int payload = -1;    //0:mulaw, 8:alaw


static short search(short val, short *table, short size)
{
   short i;
   
   for (i = 0; i < size; i++) {
      if (val <= *table++)
	 return (i);
   }
   return (size);
}



unsigned char linear2alaw(short pcm_val)	/* 2's complement (16-bit range) */
{
   short	 mask;
   short	 seg;
   unsigned char aval;
   
   pcm_val = pcm_val >> 3;

   if (pcm_val >= 0) {
      mask = 0xD5;		/* sign (7th) bit = 1 */
   } else {
      mask = 0x55;		/* sign bit = 0 */
      pcm_val = -pcm_val - 1;
   }
   
   /* Convert the scaled magnitude to segment number. */
   seg = search(pcm_val, seg_aend, 8);
   
   /* Combine the sign, segment, and quantization bits. */
   
   if (seg >= 8)		/* out of range, return maximum value. */
      return (unsigned char) (0x7F ^ mask);
   else {
      aval = (unsigned char) seg << SEG_SHIFT;
      if (seg < 2)
	 aval |= (pcm_val >> 1) & QUANT_MASK;
      else
	 aval |= (pcm_val >> seg) & QUANT_MASK;
      return (aval ^ mask);
   }
}

unsigned char linear2ulaw(short pcm_val)	/* 2's complement (16-bit range) */
{
   short         mask;
   short	 seg;
   unsigned char uval;
   
   /* Get the sign and the magnitude of the value. */
   pcm_val = pcm_val >> 2;
   if (pcm_val < 0) {
      pcm_val = -pcm_val;
      mask = 0x7F;
   } else {
      mask = 0xFF;
   }
   if ( pcm_val > CLIP ) pcm_val = CLIP;		/* clip the magnitude */
   pcm_val += (BIAS >> 2);
   
   /* Convert the scaled magnitude to segment number. */
   seg = search(pcm_val, seg_uend, 8);
   
   /*
   * Combine the sign, segment, quantization bits;
   * and complement the code word.
   */
   if (seg >= 8)		/* out of range, return maximum value. */
      return (unsigned char) (0x7F ^ mask);
   else {
      uval = (unsigned char) (seg << 4) | ((pcm_val >> (seg + 1)) & 0xF);
      return (uval ^ mask);
   }
}


int main(int argc, char * argv[])
{
    int opt;
    string codec("");
    while ((opt = getopt(argc, argv, "hi:c:o:")) != -1)
    {
        switch (opt)
        {
            case 'h':
                usage();
                exit(1);
            case 'i':
                input = (string)optarg;
                break;
            case 'o':
                output = (string)optarg;
                break;
            case 'c':
                codec = (string)optarg;
                break;
        }
    }
    if(0 == codec.compare("alaw")) payload = 8;
    else if(0 == codec.compare("mulaw")) payload = 0;
    else{
        cout <<"no codec" << endl;
        usage();
        exit(1);
    }
    if (input.length() == 0 || output.length() == 0){
        cout <<"no filename" << endl;
        usage();
        exit(1);
    }
    do_process();
    return 0;
}


void usage()
{
    fprintf(stderr, "===== Usage =====\n");
    fprintf(stderr, "-i: pcm output name \n");
    fprintf(stderr, "-o: G.711 file  name  \n");
    fprintf(stderr, "-c: codec name (alaw or mulaw)\n");
    fprintf(stderr, "./linear_g711 -i output.pcm -o input.alaw -c alaw  \n");
}

void do_process()
{
    FILE *fh = fopen(input.c_str(), "r");
    if(!fh){
        fprintf(stderr, "cannot open file[%s]\n",input.c_str());
        exit(0);        
    }
    FILE *fh_711 = fopen(output.c_str(), "w+");
    if(!fh_711){
        fprintf(stderr, "cannot open file[%s]\n",input.c_str());
        fclose(fh);
        exit(0);        
    }

    u_char w;
    int16_t r;
    long lSize;

	fseek (fh , 0 , SEEK_END);
	lSize = ftell (fh) / sizeof(int16_t);
	rewind (fh);
    for(long x = 0; x < lSize; x++){
        fread(&r, sizeof(int16_t), 1, fh);
        if(0 == payload) w = linear2ulaw(r);
        else w = linear2alaw(r);
        fwrite(&w, sizeof(u_char), 1, fh_711);
    }
    
    
    fclose(fh);
    fclose(fh_711);
}