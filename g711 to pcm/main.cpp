#include <stdio.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <iostream>
#include <list>
#include <string>

using namespace std;

void usage();
void do_process();

string input(""), output("");
int payload = -1;    //0:mulaw, 8:alaw

const int QUANT_MASK = 0xf;  /* Quantization field mask. */
const int NSEGS = 8;         /* Number of A-law segments. */
const int SEG_SHIFT = 4;     /* Left shift for segment number. */
const int SEG_MASK = 0x70;   /* Segment field mask. */
const int SIGN_BIT = 0x80;	/* Sign bit for a A-law byte. */
const int G711_SAMPLES_PER_FRAME = 160;
const int TABLE_SIZE = 8;
const int BIAS = 0x84;		/* Bias for linear code. */
const int CLIP = 8159;

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


int main(int argc, char *argv[])
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
    fprintf(stderr, "-f: G.711 file  name  \n");
    fprintf(stderr, "-o: pcm output name \n");
    fprintf(stderr, "-c: codec name (alaw or mulaw)\n");
    fprintf(stderr, "./g711_2_pcm -f input.alaw -c alaw  \n");
}



void do_process()
{
    FILE *fh = fopen(input.c_str(), "r");
    if(!fh){
        fprintf(stderr, "cannot open file[%s]\n",input.c_str());
        exit(0);        
    }
    FILE *fh_pcm = fopen(output.c_str(), "w+");
    if(!fh_pcm){
        fprintf(stderr, "cannot open file[%s]\n",input.c_str());
        fclose(fh);
        exit(0);        
    }

    u_char r;
    int16_t w;
    long lSize;

	fseek (fh , 0 , SEEK_END);
	lSize = ftell (fh);
	rewind (fh);
    for(long x = 0; x < lSize; x++){
        fread(&r, sizeof(u_char), 1, fh);
        if(0 == payload) w = ulaw2linear(r);
        else w = alaw2linear(r);
        fwrite(&w, sizeof(int16_t), 1, fh_pcm);
    }
    
    
    fclose(fh);
    fclose(fh_pcm);
}